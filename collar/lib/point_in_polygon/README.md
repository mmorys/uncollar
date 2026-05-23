# Point-in-Polygon Library

A lightweight, ESP32-optimized library for determining whether a geographic point lies within a polygon boundary. Designed for the Uncollar GPS dog collar project.

## Features

- **Efficient**: Uses ray casting (even-odd rule) algorithm with O(n) complexity
- **ESP32 Optimized**: Uses `float` (32-bit) for hardware FPU acceleration
- **Low Memory**: No dynamic allocation; polygon stores pointer to external array
- **Flexible**: Supports both convex and concave polygons
- **Bounding Box Optimization**: Quick rejection for distant points

## Quick Start

### Basic Usage

```cpp
#include <point_in_polygon.h>

// Define polygon vertices (e.g., a backyard boundary)
GeoPoint backyard[] = {
    {40.7120f, -74.0070f},  // Southwest
    {40.7120f, -74.0060f},  // Southeast
    {40.7130f, -74.0060f},  // Northeast
    {40.7130f, -74.0070f}   // Northwest
};

// Create polygon
Polygon fence(backyard, 4);

// Check if dog is within boundary
GeoPoint dogLocation = {40.7125f, -74.0065f};
if (fence.contains(dogLocation)) {
    // Dog is safe within the yard
} else {
    // Dog has escaped! Send alert
}
```

### Standalone Function

For a functional approach:

```cpp
#include <point_in_polygon.h>

GeoPoint point = {40.7125f, -74.0065f};
GeoPoint vertices[] = { /* ... */ };

if (pointInPolygon(point, vertices, 4)) {
    // Point is inside
}
```

## API Reference

### GeoPoint Struct

```cpp
struct GeoPoint {
    float lat;  // Latitude in decimal degrees (-90 to 90)
    float lon;  // Longitude in decimal degrees (-180 to 180)
};
```

### Polygon Class

#### Constructor

```cpp
Polygon(const GeoPoint* vertices, size_t count);
```

| Parameter | Description |
|-----------|-------------|
| `vertices` | Pointer to array of GeoPoint vertices (must remain valid) |
| `count` | Number of vertices (minimum 3) |

#### Methods

| Method | Return | Description |
|--------|--------|-------------|
| `contains(point)` | `bool` | Check if point is inside polygon |
| `vertexCount()` | `size_t` | Get number of vertices |
| `minLat()` | `float` | Get bounding box minimum latitude |
| `maxLat()` | `float` | Get bounding box maximum latitude |
| `minLon()` | `float` | Get bounding box minimum longitude |
| `maxLon()` | `float` | Get bounding box maximum longitude |

### Standalone Function

```cpp
bool pointInPolygon(const GeoPoint& point, 
                    const GeoPoint* vertices, 
                    size_t vertexCount);
```

## Algorithm

The library uses the **Ray Casting (Even-Odd Rule)** algorithm:

1. Cast a horizontal ray from the test point to the right
2. Count how many polygon edges the ray crosses
3. Odd count → inside; Even count → outside

### Optimizations

1. **Bounding Box Pre-check**: Points outside the bounding box are rejected immediately
2. **Float Precision**: Uses ESP32 hardware FPU for fast floating-point operations
3. **No Dynamic Allocation**: Polygon stores pointer to external array

## Performance

| Polygon Size | Est. Time @ 240MHz |
|--------------|-------------------|
| 10 vertices | ~1 μs |
| 50 vertices | ~5 μs |
| 100 vertices | ~10 μs |

*Actual performance depends on bounding box hit rate and compiler optimizations.*

## Notes

- **Boundary Behavior**: Points exactly on the boundary or vertex are considered **inside**. This is acceptable for GPS geofencing where exact boundary hits are extremely rare due to GPS precision limits (~3-5m accuracy).
- **Vertex Order**: Vertices should be ordered consistently (clockwise or counter-clockwise)
- **Closed Polygon**: The polygon is implicitly closed; do not duplicate the first vertex
- **Memory**: The vertex array must remain valid for the lifetime of the Polygon object

## Testing

Run unit tests:

```bash
pio test -e native
```

## License

Apache 2.0 License
