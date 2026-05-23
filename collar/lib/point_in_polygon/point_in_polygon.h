/**
 * @file point_in_polygon.h
 * @brief Lightweight, ESP32-optimized library for point-in-polygon testing.
 * 
 * This library provides efficient geographic point-in-polygon testing
 * optimized for the ESP32 microcontroller's hardware FPU. It uses the
 * ray casting (even-odd rule) algorithm with bounding box optimization.
 * 
 * @copyright Apache 2.0 License
 */

#ifndef POINT_IN_POLYGON_H
#define POINT_IN_POLYGON_H

#include <stddef.h>

/**
 * @brief Represents a geographic coordinate in decimal degrees.
 * 
 * Uses float (32-bit) for ESP32 hardware FPU acceleration.
 * Precision: ~1.1 meters at equator, sufficient for GPS geofencing.
 */
struct GeoPoint {
    float lat;  ///< Latitude in decimal degrees (-90 to 90)
    float lon;  ///< Longitude in decimal degrees (-180 to 180)
};

/**
 * @brief Represents a closed polygon boundary for geofencing.
 * 
 * The polygon stores a pointer to an external vertex array - no ownership
 * or copying is performed. This design minimizes memory usage and avoids
 * dynamic allocation on the ESP32.
 * 
 * Supports both convex and concave polygons.
 * 
 * @note The vertex array must remain valid for the lifetime of the Polygon.
 * @note Polygons are implicitly closed (last vertex connects to first).
 */
class Polygon {
public:
    /**
     * @brief Construct a polygon from an array of vertices.
     * 
     * @param vertices Pointer to array of GeoPoint vertices.
     *                 Must remain valid for the lifetime of this Polygon.
     * @param count    Number of vertices in the array (minimum 3).
     * 
     * @note Vertices should be ordered consistently (clockwise or counter-clockwise).
     * @note The polygon is implicitly closed; do not duplicate the first vertex at the end.
     */
    Polygon(const GeoPoint* vertices, size_t count);

    /**
     * @brief Check if a point is inside the polygon.
     * 
     * Uses the ray casting (even-odd rule) algorithm with bounding box
     * pre-check for optimization.
     * 
     * @param point The geographic point to test.
     * @return true if the point is inside the polygon, false otherwise.
     * 
     * @note Points exactly on the boundary or vertex are considered inside.
     *       This is acceptable for GPS geofencing where exact boundary hits
     *       are extremely rare due to GPS precision limits (~3-5m accuracy).
     */
    bool contains(const GeoPoint& point) const;

    /**
     * @brief Get the number of vertices in the polygon.
     */
    size_t vertexCount() const;

    /**
     * @brief Get the minimum latitude of the bounding box.
     */
    float minLat() const;

    /**
     * @brief Get the maximum latitude of the bounding box.
     */
    float maxLat() const;

    /**
     * @brief Get the minimum longitude of the bounding box.
     */
    float minLon() const;

    /**
     * @brief Get the maximum longitude of the bounding box.
     */
    float maxLon() const;

private:
    const GeoPoint* _vertices;  ///< Pointer to external vertex array (no ownership)
    size_t _count;              ///< Number of vertices
    float _minLat;              ///< Bounding box minimum latitude
    float _maxLat;              ///< Bounding box maximum latitude
    float _minLon;              ///< Bounding box minimum longitude
    float _maxLon;              ///< Bounding box maximum longitude

    /**
     * @brief Compute the bounding box from vertices.
     * Called once during construction.
     */
    void computeBoundingBox();
};

/**
 * @brief Standalone function for point-in-polygon testing.
 * 
 * Alternative API for users who prefer a functional approach.
 * 
 * @param point       The geographic point to test.
 * @param vertices    Pointer to array of GeoPoint vertices.
 * @param vertexCount Number of vertices in the array (minimum 3).
 * @return true if the point is inside the polygon, false otherwise.
 * 
 * @note Points exactly on the boundary or vertex are considered outside.
 */
bool pointInPolygon(const GeoPoint& point, 
                    const GeoPoint* vertices, 
                    size_t vertexCount);

#endif // POINT_IN_POLYGON_H
