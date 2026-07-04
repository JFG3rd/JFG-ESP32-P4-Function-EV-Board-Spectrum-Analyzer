/* WiFi manager — see net_mgr.h for the behavioral contract.
 *
 * Runs on the ESP32-P4 through esp_wifi_remote: the standard esp_wifi
 * API is proxied over SDIO to the on-board ESP32-C6 (ESP-Hosted slave). */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mdns.h"
#include "esp_hosted.h"
#include "net_mgr.h"

static const char *TAG = "net_mgr";

#define NVS_NS_WIFI   "wifi"
#define KEY_SSID      "ssid"
#define KEY_PASS      "pass"

#define STA_MAX_RETRY 3
#define SCAN_MAX      20

typedef enum { NET_OFF, NET_JOINING, NET_STA_UP, NET_AP_UP } net_state_t;

static net_state_t       s_state = NET_OFF;
static SemaphoreHandle_t s_lock;
static int               s_retry;
static char              s_sta_ssid[NET_SSID_MAX];
static char              s_ip_str[16] = "";
static char              s_ap_ssid[NET_SSID_MAX];
static char              s_ap_pass[16];
static bool              s_scanning;
static char              s_scan_ssids[SCAN_MAX][NET_SSID_MAX];
static int               s_scan_count;
static bool              s_mdns_up;

/* ── setup AP identity from the eFuse MAC ─────────────────────── */

static void derive_ap_identity(void)
{
    /* Base eFuse MAC: the P4 has no native radio, so the WIFI_SOFTAP
     * derived MAC is all zeros here — the base MAC is always valid. */
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_BASE);
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "SpectrumAnalyzer-%02X%02X", mac[4], mac[5]);
    /* WPA2 needs >= 8 chars; stable per device so it can be printed on
     * the settings screen and in the manual */
    snprintf(s_ap_pass, sizeof(s_ap_pass), "SA-%02X%02X%02X%02X",
             mac[2], mac[3], mac[4], mac[5]);
}

/* ── scan result dedup ────────────────────────────────────────── */

static void process_scan_results(void)
{
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > 64) n = 64;

    wifi_ap_record_t *recs = calloc(n ? n : 1, sizeof(wifi_ap_record_t));
    if (!recs) { s_scanning = false; return; }
    esp_wifi_scan_get_ap_records(&n, recs);

    /* dedup by SSID keeping the strongest RSSI */
    typedef struct { char ssid[NET_SSID_MAX]; int8_t rssi; } uniq_t;
    static uniq_t uniq[SCAN_MAX];
    int cnt = 0;

    for (uint16_t i = 0; i < n; i++) {
        const char *ssid = (const char *)recs[i].ssid;
        if (ssid[0] == '\0') continue;              /* hidden */
        int j;
        for (j = 0; j < cnt; j++)
            if (strncmp(uniq[j].ssid, ssid, NET_SSID_MAX) == 0) break;
        if (j < cnt) {
            if (recs[i].rssi > uniq[j].rssi) uniq[j].rssi = recs[i].rssi;
        } else if (cnt < SCAN_MAX) {
            strlcpy(uniq[cnt].ssid, ssid, NET_SSID_MAX);
            uniq[cnt].rssi = recs[i].rssi;
            cnt++;
        }
    }
    free(recs);

    /* sort by RSSI descending (tiny N — insertion sort) */
    for (int i = 1; i < cnt; i++) {
        uniq_t key = uniq[i];
        int j = i - 1;
        while (j >= 0 && uniq[j].rssi < key.rssi) { uniq[j + 1] = uniq[j]; j--; }
        uniq[j + 1] = key;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_scan_count = cnt;
    for (int i = 0; i < cnt; i++)
        strlcpy(s_scan_ssids[i], uniq[i].ssid, NET_SSID_MAX);
    s_scanning = false;
    xSemaphoreGive(s_lock);

    ESP_LOGI(TAG, "scan done: %d unique networks", cnt);
}

/* ── setup AP ─────────────────────────────────────────────────── */

static void start_setup_ap(void)
{
    wifi_config_t ap_cfg = { 0 };
    strlcpy((char *)ap_cfg.ap.ssid, s_ap_ssid, sizeof(ap_cfg.ap.ssid));
    strlcpy((char *)ap_cfg.ap.password, s_ap_pass, sizeof(ap_cfg.ap.password));
    ap_cfg.ap.ssid_len       = strlen(s_ap_ssid);
    ap_cfg.ap.channel        = 1;
    ap_cfg.ap.authmode       = WIFI_AUTH_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;

    /* APSTA so the provisioning portal can still run SSID scans */
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);

    s_state = NET_AP_UP;
    strlcpy(s_ip_str, "192.168.4.1", sizeof(s_ip_str));
    ESP_LOGI(TAG, "setup AP up: SSID '%s'  password '%s'  http://192.168.4.1",
             s_ap_ssid, s_ap_pass);
}

/* ── events ───────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            if (s_state == NET_JOINING) esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_state == NET_JOINING && ++s_retry <= STA_MAX_RETRY) {
                ESP_LOGW(TAG, "join failed, retry %d/%d", s_retry, STA_MAX_RETRY);
                esp_wifi_connect();
            } else if (s_state == NET_JOINING) {
                ESP_LOGW(TAG, "could not join '%s' — falling back to setup AP", s_sta_ssid);
                start_setup_ap();
            } else if (s_state == NET_STA_UP) {
                ESP_LOGW(TAG, "WiFi dropped — reconnecting");
                s_state = NET_JOINING;
                s_retry = 0;
                esp_wifi_connect();
            }
            break;
        case WIFI_EVENT_SCAN_DONE:
            process_scan_results();
            break;
        default:
            break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&ev->ip_info.ip));
        s_state = NET_STA_UP;
        s_retry = 0;
        ESP_LOGI(TAG, "connected to '%s' — http://%s/", s_sta_ssid, s_ip_str);

        if (!s_mdns_up && mdns_init() == ESP_OK) {
            mdns_hostname_set("spectrumanalyzer");
            mdns_instance_name_set("ESP32-P4 Spectrum Analyzer");
            mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
            s_mdns_up = true;
            ESP_LOGI(TAG, "mDNS: http://spectrumanalyzer.local/");
        }
    }
}

/* ── credentials ──────────────────────────────────────────────── */

static bool load_credentials(char *ssid, size_t ssid_len,
                             char *pass, size_t pass_len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS_WIFI, NVS_READONLY, &h) != ESP_OK) return false;
    size_t sl = ssid_len, pl = pass_len;
    bool ok = (nvs_get_str(h, KEY_SSID, ssid, &sl) == ESP_OK) &&
              (nvs_get_str(h, KEY_PASS, pass, &pl) == ESP_OK);
    nvs_close(h);
    return ok && ssid[0] != '\0';
}

static void reboot_timer_cb(void *arg)
{
    (void)arg;
    esp_restart();
}

esp_err_t net_mgr_save_credentials(const char *ssid, const char *pass)
{
    ESP_RETURN_ON_FALSE(ssid && ssid[0] && strlen(ssid) <= 32,
                        ESP_ERR_INVALID_ARG, TAG, "bad ssid");
    ESP_RETURN_ON_FALSE(pass == NULL || strlen(pass) <= 63,
                        ESP_ERR_INVALID_ARG, TAG, "bad password");

    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NS_WIFI, NVS_READWRITE, &h), TAG, "nvs open");
    esp_err_t err = nvs_set_str(h, KEY_SSID, ssid);
    if (err == ESP_OK) err = nvs_set_str(h, KEY_PASS, pass ? pass : "");
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    ESP_RETURN_ON_ERROR(err, TAG, "nvs write");

    ESP_LOGI(TAG, "credentials saved for '%s' — rebooting to join", ssid);
    const esp_timer_create_args_t targs = {
        .callback = reboot_timer_cb, .name = "wifi_reboot",
    };
    esp_timer_handle_t t;
    if (esp_timer_create(&targs, &t) == ESP_OK)
        esp_timer_start_once(t, 1500 * 1000);
    return ESP_OK;
}

/* ── public API ───────────────────────────────────────────────── */

esp_err_t net_mgr_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_lock, ESP_ERR_NO_MEM, TAG, "mutex");

    derive_ap_identity();

    /* Bring up the ESP-Hosted transport (SDIO link to the on-board C6)
     * BEFORE any esp_wifi call — esp_wifi_remote proxies over it. */
    int hres = esp_hosted_init();
    if (hres != 0) {
        ESP_LOGW(TAG, "esp_hosted_init failed (%d) — is the C6 slave firmware present?", hres);
        return ESP_FAIL;
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&init_cfg);
    if (err != ESP_OK) {
        /* esp-hosted transport (SDIO to the C6) not available */
        ESP_LOGW(TAG, "esp_wifi_init failed (%s) — WiFi disabled", esp_err_to_name(err));
        return err;
    }

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    char pass[64] = "";
    bool have_creds = load_credentials(s_sta_ssid, sizeof(s_sta_ssid),
                                       pass, sizeof(pass));

    if (have_creds) {
        wifi_config_t sta_cfg = { 0 };
        strlcpy((char *)sta_cfg.sta.ssid, s_sta_ssid, sizeof(sta_cfg.sta.ssid));
        strlcpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password));
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
        s_state = NET_JOINING;
        s_retry = 0;
        ESP_LOGI(TAG, "joining '%s'...", s_sta_ssid);
    } else {
        start_setup_ap();
    }

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start");
    return ESP_OK;
}

bool net_mgr_is_sta_connected(void)
{
    return s_state == NET_STA_UP;
}

void net_mgr_get_status(char *buf, size_t len)
{
    if (!buf || len == 0) return;
    switch (s_state) {
    case NET_STA_UP:
        snprintf(buf, len, "WiFi: %s  %s", s_sta_ssid, s_ip_str);
        break;
    case NET_JOINING:
        snprintf(buf, len, "WiFi: joining %s...", s_sta_ssid);
        break;
    case NET_AP_UP:
        snprintf(buf, len, "AP %s  pw %s  http://192.168.4.1", s_ap_ssid, s_ap_pass);
        break;
    default:
        strlcpy(buf, "WiFi: off", len);
        break;
    }
}

esp_err_t net_mgr_start_scan(void)
{
    ESP_RETURN_ON_FALSE(s_state != NET_OFF, ESP_ERR_INVALID_STATE, TAG, "wifi off");
    if (s_scanning) return ESP_OK;
    s_scanning = true;
    esp_err_t err = esp_wifi_scan_start(NULL, false);   /* async */
    if (err != ESP_OK) {
        s_scanning = false;
        ESP_LOGW(TAG, "scan start failed: %s", esp_err_to_name(err));
    }
    return err;
}

int net_mgr_get_scan_results(char names[][NET_SSID_MAX], int max, bool *in_progress)
{
    if (in_progress) *in_progress = s_scanning;
    if (!names || max <= 0) return 0;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    int n = s_scan_count < max ? s_scan_count : max;
    for (int i = 0; i < n; i++)
        strlcpy(names[i], s_scan_ssids[i], NET_SSID_MAX);
    xSemaphoreGive(s_lock);
    return n;
}
