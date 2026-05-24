# Uncollar — Base Station Firmware

The base station is an always-on ESP32 relay between Home Assistant and the collar. It receives `PositionReport` and `BoundaryAlert` LoRa packets from the collar and exposes them as native Home Assistant entities. It can send `ConfigUpdate` packets back to the collar to update the geofence.

## Hardware

- **Microcontroller**: Adafruit QT Py ESP32 Pico (ESP32-PICO-MINI-02)
- **Radio**: RFM95W 915 MHz LoRa transceiver (SX1276-based)

See [WIRING.md](WIRING.md) for the full pin-by-pin connection diagram.

## Stack

| Layer | Technology |
|---|---|
| Firmware | [ESPHome](https://esphome.io/) |
| Radio | ESPHome [`sx127x`](https://esphome.io/components/sx127x/) component |
| Home Assistant | ESPHome native API (auto-discovery, no MQTT config required) |
| OTA updates | ESPHome built-in |
| Credentials | `basestation/esphome/secrets.yaml` (gitignored) |

## ESPHome Configuration

| File | Purpose | Committed |
|---|---|---|
| `esphome/basestation.yaml` | Full ESPHome config | Yes |
| `esphome/secrets.yaml` | WiFi, OTA password, LoRa sync word | **No — gitignored** |
| `esphome/secrets.yaml.example` | Template for the above | Yes |
| `esphome/uncollar_protocol.h` | Wire struct definitions for `on_packet` lambda | Yes |

### First-time setup

1. Copy `esphome/secrets.yaml.example` to `esphome/secrets.yaml` and fill in your values.
2. Copy `uncollar_protocol.h` into your ESPHome container's config directory alongside `basestation.yaml`.
3. Install and compile via the ESPHome dashboard or CLI:
   ```bash
   esphome compile esphome/basestation.yaml
   esphome upload  esphome/basestation.yaml
   ```

## LoRa Protocol

Wire message types are defined in [collar/lib/radio/radio.h](../collar/lib/radio/radio.h) and mirrored in `esphome/uncollar_protocol.h` for use in ESPHome lambdas.

| Message | Direction | Purpose |
|---|---|---|
| `PositionReport` | Collar → Base | Lat/lon, satellites, inside/outside boundary, battery |
| `BoundaryAlert` | Collar → Base | Immediate notification on boundary crossing |
| `ConfigUpdate` | Base → Collar | Updated home position and geofence vertices |

### LoRa RF parameters

Both sides must use identical parameters:

| Parameter | Value |
|---|---|
| Frequency | 915 MHz |
| Bandwidth | 125 kHz |
| Spreading factor | 9 |
| Coding rate | 4/7 |
| Sync word | set in `secrets.yaml` / `collar/credentials.ini` |
| TX power | 17 dBm |
| Preamble | 8 symbols |

## Home Assistant Entities

Once added to HA, the basestation exposes:

| Entity | Type | Description |
|---|---|---|
| Dog Latitude | Sensor | Last reported latitude (°) |
| Dog Longitude | Sensor | Last reported longitude (°) |
| Dog GPS Satellites | Sensor | Satellite count at last fix |
| Collar Battery | Sensor | Battery voltage (mV) |
| LoRa RSSI | Sensor | Signal strength of last received packet (dBm) |
| Dog Inside Boundary | Binary sensor | Current geofence status |
| Boundary Alert | Binary sensor | True when dog has exited the boundary |
