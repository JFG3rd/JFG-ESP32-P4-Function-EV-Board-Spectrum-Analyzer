#pragma once
#include "esp_err.h"

/* On-device HTTP server (Phase 2). Serves the WiFi provisioning portal,
 * the mic-calibration upload page, and a small status API. Start after
 * net_mgr_init() — works in both setup-AP and station mode. */
esp_err_t web_server_start(void);
