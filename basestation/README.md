# Uncollar — Base Station Firmware

This directory will contain the base station firmware for the Uncollar system.

## Planned Stack

- **Hardware**: ESP-WROOM-32 ESP32S with RFM95W 915 MHz LoRa transceiver
- **Framework**: Arduino / PlatformIO
- **Communication**: LoRa (receives position reports from collar), WiFi + MQTT (publishes to Home Assistant)

## Planned Responsibilities

- Receive `PositionReport` and `BoundaryAlert` LoRa packets from the collar (see `collar/lib/radio/radio.h` for wire protocol)
- Publish location and alert data to Home Assistant via MQTT
- Accept geofence and home-position updates from Home Assistant and relay them to the collar as `ConfigUpdate` packets
- Maintain MQTT discovery payloads so the device auto-registers with Home Assistant

## Development Status

Not yet started. Wire message types are defined in [collar/lib/radio/radio.h](../collar/lib/radio/radio.h).
