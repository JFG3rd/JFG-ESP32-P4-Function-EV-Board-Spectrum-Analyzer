## [Unreleased]
### Added
- (none yet)

## [1.1.0] - 2026-07-11
### Added
- Wi-Fi: remember multiple networks (up to 8, most-recently-used) with
  automatic reconnect to whichever known network is in range; migrates the
  previous single-credential storage on first boot.
- Wi-Fi: robust on-device provisioning — the SSID scan no longer conflicts
  with the join loop, plus an 8 s scan timeout so the setup screen never
  hangs on "Scanning...".
- Wi-Fi: "Show password" toggle on the on-device manual entry screen.
- Wi-Fi: per-device mDNS hostname (`spectralab-p4-xxxx.local`) so multiple
  units on one LAN no longer collide.
- Wi-Fi: verbose network-manager diagnostics — connection state-machine
  transition tracing and decoded disconnect reason codes for debugging
  join failures.
- Automatic Gain Control (AGC): optional software auto-gain for long
  unattended sessions. Hybrid actuator — coarse ES8311 hardware PGA
  (6 dB steps) plus a continuous software trim in the DSP input stage;
  falls back to software-only trim for USB mics. Runtime-adjustable
  target level and speed (Slow/Medium/Fast), an on-screen `AGC` toggle
  button, and a Settings group. Manual override: changing Mic gain in
  Settings disables the AGC immediately. New `components/agc`.
- Display-mode title shown in the spectrum status bar (top-right).
- Vertical frequency grid overlay on the waterfall, toggled by `GRD`.
- `GRD` button now shows a check mark when the grid is on.
- On-device camera QR Wi-Fi provisioning flow:
  - `Scan QR` path in Wi-Fi setup UI
  - camera-backed QR decode pipeline (`qr_scan` component)
  - live camera preview on the scan screen
  - decoded SSID/password routed into existing password/save flow

### Changed
- Grid lines now draw on top of the bars in all band modes so the
  frequency/dB graticule stays visible regardless of bar height.

### Fixed
- Touch: GT911 init now probes both I2C addresses (0x5D and 0x14) with a
  short retry instead of assuming 0x5D. On this board the touch INT pin is
  not connected, so the controller's power-on I2C address can latch either
  value; the old single-address path could silently disable the touchscreen
  while the display kept working.
- dB legend was painted over by the leftmost bars; it now renders on
  top of the spectrum with a background chip for legibility.
- Build reliability: fixed clean-build race around generated `esp_ipa`
  sources by adding deterministic pre-generation.
- QR/camera runtime stability:
  - increased scanner task stack headroom
  - safer camera format negotiation + fallback handling
  - camera auto-adjust controls (auto white balance / auto gain / auto exposure,
    plus brightness/contrast control requests)
- Touch behavior during active camera scan:
  - suppress GT911 polling while QR scan is running to avoid I2C contention
  - prevent repeated touch read-error storms while camera provisioning is active

## [1.0.0] - 2026-07-07
### Added
- Initial stable release of SpectraLab-P4
- Embedded web interface for control/visualization

### Notes
- First production-ready milestone
