# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Uncollar is a monorepo for an open-source GPS dog collar system. Three components live here:

| Directory | Language | Status |
|---|---|---|
| `collar/` | C++ / Arduino / PlatformIO | Active |
| `basestation/` | C++ / Arduino / PlatformIO | Planned |
| `homeassistant/` | Python | Planned |

The collar tracks GPS position, checks it against a configurable geofence polygon, and communicates over LoRa to the base station, which integrates with Home Assistant via MQTT.

## Commands

All PlatformIO commands are run from the `collar/` directory (or with `--project-dir collar/` from the repo root).

```bash
# Build firmware (adafruit_qtpy_esp32s3_nopsram; native env prints "Nothing to build" — expected)
pio run --project-dir collar/

# Upload to device (Adafruit QT Py ESP32-S3)
pio run --project-dir collar/ --target upload

# Monitor serial output (115200 baud)
pio device monitor --project-dir collar/

# Run host-side unit tests (no hardware needed)
pio test -e native --project-dir collar/

# Run all tests (native + arduino-specific)
pio test --project-dir collar/

# Clean build artifacts
pio run --project-dir collar/ --target clean
```

## Collar Firmware Architecture

**Execution model**: The firmware runs entirely in `setup()` — `loop()` is never reached. Each wake cycle from deep sleep re-runs `setup()`, acquires a GPS fix, checks the geofence, then re-enters deep sleep via `esp_deep_sleep_start()`. Last known position survives deep sleep in `RTC_DATA_ATTR` memory.

**Two PlatformIO environments**:
- `adafruit_qtpy_esp32s3_nopsram` — target hardware (ESP32-S3); runs `test_arduino` tests only
- `native` — host machine; runs `test_native*` tests only (no Arduino dependencies)

**Library layout** (`collar/lib/`): Each reusable component lives in its own subdirectory with a `.h`/`.cpp` pair. Every subsystem exposes a pure-virtual interface (prefix `I`) so concrete implementations can be swapped or mocked in native tests. Prefer adding code here over expanding `collar/src/main.cpp`.

- `collar/lib/point_in_polygon/` — `GeoPoint` struct + `Polygon` class. Ray casting (even-odd rule) with bounding box pre-check. `Polygon` does not own its vertex array; the caller must keep the array alive.
- `collar/lib/config_manager/` — `IConfigManager` interface + `ConfigManager` implementation. Wraps ESP32 `Preferences` (NVS) for persistent storage of the default GPS position and geofence boundary vertices. Loads defaults on first boot; LoRa config updates call setters then `save()`.
- `collar/lib/gps_manager/` — `IGpsManager` interface + `AdafruitGpsManager` implementation (Arduino-only, `#ifdef ARDUINO`). Also exports `ddmmToDecimal()` — a platform-independent NMEA conversion utility used by native unit tests.
- `collar/lib/power_manager/` — `IPowerManager` interface + `Esp32PowerManager` implementation. Encapsulates WiFi/BT shutdown and `esp_deep_sleep_start()`.
- `collar/lib/radio/` — `IRadio` interface + binary wire message types (`PositionReport`, `BoundaryAlert`, `ConfigUpdate`) + `RFM95Radio` stub. Message structs are pure C++ (no Arduino dependency) so they can be shared with base-station code.

**Debug output**: Controlled by compile-time defines in `collar/src/main.cpp`:
- `#define DEBUG_SERIAL` — enables Serial output (enabled by default)
- `#define DEBUG_LCD` — enables I2C LCD output (disabled by default)

**I2C bus**: GPS and LCD share `Wire1` on pins 41 (SDA) and 40 (SCL). GPS address `0x10`, LCD address `0x27`.

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
