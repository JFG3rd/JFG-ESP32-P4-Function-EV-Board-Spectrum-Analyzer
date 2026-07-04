# Web assets (Phase 2)

Source files for the on-device web server. These are embedded into the
firmware via `EMBED_FILES` when milestone M3 (WiFi provisioning portal)
and M4 (REST API / web UI) are implemented — they are not yet referenced
by the build.

| File | Origin | Purpose |
|------|--------|---------|
| `style.css` | Copied verbatim from [JFG3rd/HomeKitKnock-S3](https://github.com/JFG3rd/HomeKitKnock-S3) `data/style.css` | Shared stylesheet for all device web pages |
| `wifi-setup.html` | Adapted from the same repo's `data/wifi-setup.html` (rebranded, same endpoint contract) | AP-mode provisioning portal: scan SSIDs (deduplicated server-side), pick or type a network, enter password, save & connect |

Endpoint contract expected by `wifi-setup.html` (implemented in M3):

- `GET /scanWifi` — start an async scan
- `GET /wifiScanResults` — `{"ssids": ["..."], "inProgress": bool}`; SSIDs
  are de-duplicated (strongest RSSI wins) and sorted by signal strength
- `POST /saveWiFi` — JSON `{"ssid": "...", "password": "..."}`; stores
  credentials in NVS and reboots into STA join, falling back to the setup
  AP on failure
