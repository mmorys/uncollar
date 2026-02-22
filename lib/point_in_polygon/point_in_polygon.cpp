/**
 * @file point_in_polygon.cpp
 * @brief Implementation of point-in-polygon testing using ray casting algorithm.
 * 
 * @copyright Apache 2.0 License
 */

#include "point_in_polygon.h"

// ============================================================================
// Polygon Class Implementation
// ============================================================================

Polygon::Polygon(const GeoPoint* vertices, size_t count)
    : _vertices(vertices)
    , _count(count)
    , _minLat(0.0f)
    , _maxLat(0.0f)
    , _minLon(0.0f)
    , _maxLon(0.0f)
{
    if (_vertices != nullptr && _count >= 3) {
        computeBoundingBox();
    }
}

void Polygon::computeBoundingBox() {
    // Initialize with first vertex
    _minLat = _vertices[0].lat;
    _maxLat = _vertices[0].lat;
    _minLon = _vertices[0].lon;
    _maxLon = _vertices[0].lon;

    // Find min/max for both coordinates
    for (size_t i = 1; i < _count; i++) {
        if (_vertices[i].lat < _minLat) {
            _minLat = _vertices[i].lat;
        } else if (_vertices[i].lat > _maxLat) {
            _maxLat = _vertices[i].lat;
        }

        if (_vertices[i].lon < _minLon) {
            _minLon = _vertices[i].lon;
        } else if (_vertices[i].lon > _maxLon) {
            _maxLon = _vertices[i].lon;
        }
    }
}

bool Polygon::contains(const GeoPoint& point) const {
    // Invalid polygon check
    if (_vertices == nullptr || _count < 3) {
        return false;
    }

    // Bounding box pre-check (optimization)
    // Quick rejection for points clearly outside the polygon
    if (point.lat < _minLat || point.lat > _maxLat ||
        point.lon < _minLon || point.lon > _maxLon) {
        return false;
    }

    // Ray casting algorithm (even-odd rule)
    // 
    // Cast a horizontal ray from the point to the right (+longitude direction)
    // Count how many polygon edges it crosses:
    //   - Odd count = inside
    //   - Even count = outside
    //
    // The algorithm handles both convex and concave polygons correctly.
    
    bool inside = false;
    size_t j = _count - 1;  // Index of previous vertex (wraps around)

    for (size_t i = 0; i < _count; i++) {
        const GeoPoint& vi = _vertices[i];
        const GeoPoint& vj = _vertices[j];

        // Check if the ray could possibly cross this edge:
        // The edge must straddle the point's latitude
        // Using != to handle both crossing directions
        if ((vi.lat > point.lat) != (vj.lat > point.lat)) {
            // Calculate the longitude where the edge crosses the horizontal line
            // at point.lat using linear interpolation
            //
            // Formula: lon = vi.lon + (point.lat - vi.lat) * (vj.lon - vi.lon) / (vj.lat - vi.lat)
            //
            // This is derived from the line equation between two points
            
            float lonAtCrossing = vi.lon + 
                (point.lat - vi.lat) * (vj.lon - vi.lon) / (vj.lat - vi.lat);

            // If the crossing longitude is greater than the point's longitude,
            // the ray crosses this edge
            if (point.lon < lonAtCrossing) {
                inside = !inside;  // Toggle inside/outside state
            }
        }

        j = i;  // Move to next edge
    }

    return inside;
}

size_t Polygon::vertexCount() const {
    return _count;
}

float Polygon::minLat() const {
    return _minLat;
}

float Polygon::maxLat() const {
    return _maxLat;
}

float Polygon::minLon() const {
    return _minLon;
}

float Polygon::maxLon() const {
    return _maxLon;
}

// ============================================================================
// Standalone Function Implementation
// ============================================================================

bool pointInPolygon(const GeoPoint& point, 
                    const GeoPoint* vertices, 
                    size_t vertexCount) {
    // Create a temporary Polygon and use its contains method
    // This avoids code duplication while providing a convenient functional API
    Polygon polygon(vertices, vertexCount);
    return polygon.contains(point);
}
