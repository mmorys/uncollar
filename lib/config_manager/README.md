# ConfigManager Library

Configuration management for Uncollar GPS collar using ESP32 Non-Volatile Storage (NVS).

## Overview

This library provides persistent storage for:
- Default latitude and longitude
- Geofence boundary vertices

Configuration persists across power cycles using ESP32's NVS (Non-Volatile Storage). On first boot, default values are used and automatically saved. When updated via LoRa (future feature), values persist.

## Features

- **NVS Persistence**: Configuration survives power cycles and deep sleep
- **Default Fallback**: Automatically uses defaults on first boot
- **Runtime Updates**: Support for updating config via LoRa (future)
- **Memory Efficient**: Dynamic boundary vertex allocation

## Default Values

| Parameter | Default |
|-----------|---------|
| Latitude | 40.72272f |
| Longitude | -74.02116f |
| Boundary | 100m x 100m square around default location |

## Usage

### Basic Usage

```cpp
#include "config_manager.h"

// Declare config manager instance
ConfigManager configManager;

void setup() {
    // Initialize - loads from NVS or uses defaults
    if (!configManager.begin()) {
        Serial.println("Failed to initialize config!");
        return;
    }
    
    // Get configuration values
    float lat = configManager.getDefaultLatitude();
    float lon = configManager.getDefaultLongitude();
    
    // Get boundary vertices for Polygon
    const GeoPoint* vertices = configManager.getBoundaryVertices();
    size_t count = configManager.getBoundaryVertexCount();
    
    // Create polygon
    Polygon boundary(vertices, count);
}
```

### Updating Configuration (Future LoRa)

```cpp
// Example: Update boundary via LoRa command
void updateBoundary(const GeoPoint* newVertices, size_t newCount) {
    configManager.setBoundaryVertices(newVertices, newCount);
    configManager.save();  // Persist to NVS
}
```

### Reset to Defaults

```cpp
// Reset all configuration to defaults
configManager.resetToDefaults();
```

## API Reference

### Methods

| Method | Description |
|--------|-------------|
| `begin()` | Initialize and load config from NVS |
| `load()` | Load config from NVS, fallback to defaults |
| `save()` | Save current config to NVS |
| `resetToDefaults()` | Reset config to defaults and save |
| `getDefaultLatitude()` | Get default latitude |
| `getDefaultLongitude()` | Get default longitude |
| `getBoundaryVertices()` | Get boundary vertex array |
| `getBoundaryVertexCount()` | Get number of boundary vertices |
| `getConfig()` | Get full Config struct |
| `setDefaultLatitude(float)` | Set default latitude |
| `setDefaultLongitude(float)` | Set default longitude |
| `setBoundaryVertices(GeoPoint*, size_t)` | Set boundary vertices |

## NVS Keys

The following keys are used in the `uncollar_cfg` namespace:

| Key | Type | Description |
|-----|------|-------------|
| `cfg_lat` | float | Default latitude |
| `cfg_lon` | float | Default longitude |
| `cfg_bnd_cnt` | uint8_t | Boundary vertex count |
| `cfg_bnd_X_lat` | float | Vertex X latitude |
| `cfg_bnd_X_lon` | float | Vertex X longitude |

## Requirements

- ESP32 or ESP32-S3 (any ESP32 variant with NVS support)
- Arduino framework
- Preferences library (included in ESP32 Arduino core)

## File Structure

```
lib/config_manager/
├── config_manager.h    # Header with class definition
├── config_manager.cpp  # Implementation
└── README.md           # This file
```
