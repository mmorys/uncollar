#pragma once
// Standalone copy of the collar wire protocol structs for use in ESPHome lambdas.
// Keep in sync with collar/lib/radio/radio.h.
#include <stdint.h>

enum class MessageType : uint8_t {
    PositionReport = 0x01,
    BoundaryAlert  = 0x02,
    ConfigUpdate   = 0x10,
};

struct PositionReport {
    float    latitude;
    float    longitude;
    uint32_t timestamp;
    uint8_t  satellites;
    bool     insideBoundary;
    uint16_t batteryMv;
};

struct BoundaryAlert {
    float    latitude;
    float    longitude;
    uint32_t timestamp;
    bool     enteredBoundary;
};

struct GeoPoint { float lat, lon; };

struct ConfigUpdate {
    float    defaultLatitude;
    float    defaultLongitude;
    GeoPoint boundaryVertices[16];
    uint8_t  vertexCount;
};
