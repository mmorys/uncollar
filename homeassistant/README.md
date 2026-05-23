# Uncollar — Home Assistant Integration

This directory will contain the Uncollar custom component for Home Assistant.

## Planned Stack

- **Language**: Python 3
- **Integration type**: Custom component (`custom_components/uncollar/`)
- **Protocol**: MQTT (data arrives from the base station)

## Planned Responsibilities

- Expose the dog's location as a `device_tracker` entity (updates on every position report)
- Expose geofence status as a `binary_sensor` (inside / outside boundary)
- Provide a service to update the geofence polygon and home position (relayed to collar via base station)
- Send push notifications on `BoundaryAlert` events

## Development Status

Not yet started. MQTT topic schema and payload format will be defined alongside the base station firmware in [basestation/](../basestation/).
