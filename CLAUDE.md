# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Uncollar is a monorepo for an open-source GPS dog collar system. Three components live here:

| Directory | Language | Status |
|---|---|---|
| `collar/` | C++ / Arduino / PlatformIO | Active |
| `basestation/` | ESPHome YAML + C++ lambda | Active |
| `homeassistant/` | Python | Planned |

The collar tracks GPS position, checks it against a configurable geofence polygon, and communicates over LoRa to the base station. The base station runs ESPHome and exposes position data to Home Assistant via the ESPHome native API.

## Commands

### Collar (PlatformIO)

All PlatformIO commands are run from the `collar/` directory (or with `--project-dir collar/` from the repo root).

```bash
# Build firmware (mcu env; native env prints "Nothing to build" — expected)
pio run --project-dir collar/

# Upload to device (Adafruit QT Py ESP32-S3)
pio run --project-dir collar/ -e mcu --target upload

# Monitor serial output (115200 baud)
pio device monitor --project-dir collar/

# Run host-side unit tests (no hardware needed)
pio test -e native --project-dir collar/

# Clean build artifacts
pio run --project-dir collar/ --target clean

# Reassign ACM ports after replug (identifies boards by USB VID)
./assign_ports.sh
```

### Basestation (ESPHome)

The basestation is managed via an ESPHome container (web dashboard), not the CLI. Config files live in `basestation/esphome/`. The compiled YAML and `uncollar_protocol.h` must both be present in the ESPHome container's config directory.

```bash
# CLI alternative (if ESPHome is installed locally)
esphome compile basestation/esphome/basestation.yaml
esphome upload  basestation/esphome/basestation.yaml
esphome logs    basestation/esphome/basestation.yaml
```

`basestation/esphome/secrets.yaml` is gitignored. Copy `secrets.yaml.example` and fill in WiFi credentials, OTA password, API encryption key, and LoRa sync word before flashing.

## Collar Firmware Architecture

**Execution model**: The firmware runs entirely in `setup()` — `loop()` is never reached. Each wake cycle from deep sleep re-runs `setup()`, acquires a GPS fix, checks the geofence, then re-enters deep sleep via `esp_deep_sleep_start()`. Last known position survives deep sleep in `RTC_DATA_ATTR` memory.

**PlatformIO environments**:
- `mcu` — target hardware (Adafruit QT Py ESP32-S3); production firmware
- `gps_demo`, `radio_tx_demo`, `spi_raw_demo` — single-purpose hardware diagnostic sketches in `collar/demos/`
- `native` — host machine; runs `test_native*` tests only (no Arduino dependencies)

**Library layout** (`collar/lib/`): Each reusable component lives in its own subdirectory with a `.h`/`.cpp` pair. Every subsystem exposes a pure-virtual interface (prefix `I`) so concrete implementations can be swapped or mocked in native tests. Prefer adding code here over expanding `collar/src/main.cpp`.

- `collar/lib/point_in_polygon/` — `GeoPoint` struct + `Polygon` class. Ray casting (even-odd rule) with bounding box pre-check. `Polygon` does not own its vertex array; the caller must keep the array alive.
- `collar/lib/config_manager/` — `IConfigManager` interface + `ConfigManager` implementation. Wraps ESP32 `Preferences` (NVS) for persistent storage of the default GPS position and geofence boundary vertices. Loads defaults on first boot; LoRa config updates call setters then `save()`.
- `collar/lib/gps_manager/` — `IGpsManager` interface + `AdafruitGpsManager` implementation (Arduino-only, `#ifdef ARDUINO`). Also exports `ddmmToDecimal()` — a platform-independent NMEA conversion utility used by native unit tests.
- `collar/lib/power_manager/` — `IPowerManager` interface + `Esp32PowerManager` implementation. Encapsulates WiFi/BT shutdown and `esp_deep_sleep_start()`.
- `collar/lib/radio/` — `IRadio` interface + binary wire message types (`PositionReport`, `BoundaryAlert`, `ConfigUpdate`) + `RFM95Radio` implementation. Message structs are pure C++ (no Arduino dependency). The LoRa sync word is injected via the `LORA_SYNC_WORD` build flag from the gitignored `collar/credentials.ini`; falls back to `RADIOLIB_SX127X_SYNC_WORD` (0x12) if absent.

**Debug output**: Controlled by compile-time defines in `collar/src/main.cpp`:
- `#define DEBUG_SERIAL` — enables Serial output (enabled by default)
- `#define DEBUG_LCD` — enables I2C LCD output (disabled by default)

**I2C bus**: GPS and LCD share `Wire1` on pins 41 (SDA) and 40 (SCL). GPS address `0x10`, LCD address `0x27`.

## Basestation Architecture

**Stack**: ESPHome on an Adafruit QT Py ESP32 Pico (classic ESP32). No custom firmware — all WiFi, OTA, and Home Assistant integration is handled by ESPHome.

**Key files**:
- `basestation/esphome/basestation.yaml` — full ESPHome config (committed)
- `basestation/esphome/uncollar_protocol.h` — standalone copy of wire structs for the `on_packet` lambda; must be kept in sync with `collar/lib/radio/radio.h`
- `basestation/esphome/secrets.yaml` — gitignored credentials (WiFi, OTA, API key, LoRa sync word)
- `basestation/include/pins.h` — SPI and radio pin assignments for the QT Py ESP32 Pico
- `basestation/demos/spi_raw_demo.cpp` — PlatformIO diagnostic sketch (raw SPI, no RadioLib)

**Radio**: ESPHome `sx127x` component drives the RFM95W (SX1276). LoRa parameters must match the collar exactly — see `basestation/esphome/basestation.yaml` for the full set (915 MHz, SF9, 125 kHz BW, CR 4/7, 8-symbol preamble, sync word from secret).

**Hardware**: SPI on HSPI bus (GPIO 12/13/14), CS on GPIO26 (A0), RST on GPIO25 (A1), DIO0 on GPIO27 (A2). See `basestation/WIRING.md` for the full diagram.

**Credentials**: LoRa sync word lives in two gitignored files — `basestation/esphome/secrets.yaml` (`lora_sync_word`) and `collar/credentials.ini` (`-DLORA_SYNC_WORD=`). Both must use the same value.

## Naming Conventions

- Classes: `CamelCase`
- Functions: `camelCase`
- Constants/macros: `SCREAMING_SNAKE_CASE`
- Files: `snake_case`
- Test directories and files must start with `test_`

## Testing

Native tests use the Unity framework (`throwtheswitch/Unity`). Each test suite lives in its own subdirectory under `collar/tests/` — every suite gets its own binary with its own `main()` entry point. Arduino-specific tests (e.g., `test_arduino/i2c_scanner.cpp`) require real hardware and are excluded from the native environment via `test_filter`.

Native test suites (matched by `test_filter = test_native*`):
- `collar/tests/test_native/` — `Polygon` / `GeoPoint` ray-casting tests (20 cases)
- `collar/tests/test_native_gps/` — `ddmmToDecimal()` NMEA conversion tests (10 cases)
