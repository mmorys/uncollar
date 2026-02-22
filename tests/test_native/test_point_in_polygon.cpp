/**
 * @file test_point_in_polygon.cpp
 * @brief Unit tests for the point_in_polygon library.
 * 
 * Run with: pio test -e native
 * 
 * @copyright Apache 2.0 License
 */

#include <unity.h>
#include "point_in_polygon.h"

// ============================================================================
// Test Data
// ============================================================================

// Simple square polygon (clockwise order)
// Approximate coordinates for a small area
// Bounding box: lat [40.7120, 40.7130], lon [-74.0070, -74.0060]
GeoPoint squarePolygon[] = {
    {40.7120f, -74.0070f},  // Southwest
    {40.7120f, -74.0060f},  // Southeast
    {40.7130f, -74.0060f},  // Northeast
    {40.7130f, -74.0070f}   // Northwest
};
const size_t squarePolygonCount = 4;

// Concave U-shaped polygon
// Creates a "U" shape - point inside the U should be outside the polygon
GeoPoint concavePolygon[] = {
    {40.7100f, -74.0080f},  // Bottom-left
    {40.7100f, -74.0040f},  // Bottom-right
    {40.7120f, -74.0040f},  // Right-top
    {40.7120f, -74.0060f},  // Right-middle (creates the U)
    {40.7110f, -74.0060f},  // Inner-right
    {40.7110f, -74.0070f},  // Inner-left
    {40.7120f, -74.0070f},  // Left-middle (creates the U)
    {40.7120f, -74.0080f}   // Left-top
};
const size_t concavePolygonCount = 8;

// Triangle (minimum valid polygon)
GeoPoint trianglePolygon[] = {
    {40.7120f, -74.0070f},
    {40.7130f, -74.0060f},
    {40.7130f, -74.0080f}
};
const size_t trianglePolygonCount = 3;

// ============================================================================
// Basic Tests
// ============================================================================

void test_point_inside_square(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point clearly inside the square
    GeoPoint insidePoint = {40.7125f, -74.0065f};
    TEST_ASSERT_TRUE(fence.contains(insidePoint));
}

void test_point_outside_square(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point clearly outside the square (to the north)
    GeoPoint outsidePoint = {40.7140f, -74.0065f};
    TEST_ASSERT_FALSE(fence.contains(outsidePoint));
}

void test_point_outside_square_west(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point clearly outside the square (to the west)
    GeoPoint outsidePoint = {40.7125f, -74.0080f};
    TEST_ASSERT_FALSE(fence.contains(outsidePoint));
}

void test_point_outside_square_south(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point clearly outside the square (to the south)
    GeoPoint outsidePoint = {40.7110f, -74.0065f};
    TEST_ASSERT_FALSE(fence.contains(outsidePoint));
}

// ============================================================================
// Concave Polygon Tests
// ============================================================================

void test_point_inside_concave(void) {
    Polygon fence(concavePolygon, concavePolygonCount);
    
    // Point inside the main body of the U-shape (bottom area)
    GeoPoint insidePoint = {40.7105f, -74.0060f};
    TEST_ASSERT_TRUE(fence.contains(insidePoint));
}

void test_point_in_concave_cavity(void) {
    Polygon fence(concavePolygon, concavePolygonCount);
    
    // Point inside the "cavity" of the U-shape - should be OUTSIDE
    GeoPoint cavityPoint = {40.7115f, -74.0065f};
    TEST_ASSERT_FALSE(fence.contains(cavityPoint));
}

// ============================================================================
// Triangle Tests
// ============================================================================

void test_point_inside_triangle(void) {
    Polygon fence(trianglePolygon, trianglePolygonCount);
    
    // Point inside the triangle
    GeoPoint insidePoint = {40.7125f, -74.0068f};
    TEST_ASSERT_TRUE(fence.contains(insidePoint));
}

void test_point_outside_triangle(void) {
    Polygon fence(trianglePolygon, trianglePolygonCount);
    
    // Point outside the triangle
    GeoPoint outsidePoint = {40.7125f, -74.0050f};
    TEST_ASSERT_FALSE(fence.contains(outsidePoint));
}

// ============================================================================
// Boundary Cases
// ============================================================================

void test_point_on_edge(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point exactly on the bottom edge
    // Note: The ray casting algorithm considers points on edges as INSIDE
    // This is a known behavior and acceptable for GPS geofencing where
    // exact boundary hits are extremely rare due to GPS precision limits
    GeoPoint edgePoint = {40.7120f, -74.0065f};
    TEST_ASSERT_TRUE(fence.contains(edgePoint));
}

void test_point_on_vertex(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point exactly on a vertex
    // Note: The ray casting algorithm considers points on vertices as INSIDE
    // This is a known behavior and acceptable for GPS geofencing
    GeoPoint vertexPoint = {40.7120f, -74.0070f};
    TEST_ASSERT_TRUE(fence.contains(vertexPoint));
}

// ============================================================================
// Bounding Box Tests
// ============================================================================

void test_bounding_box_rejection(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point far from polygon - should be rejected by bounding box check
    GeoPoint farPoint = {40.7200f, -74.0000f};
    TEST_ASSERT_FALSE(fence.contains(farPoint));
}

void test_bounding_box_accessors(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    TEST_ASSERT_EQUAL_FLOAT(40.7120f, fence.minLat());
    TEST_ASSERT_EQUAL_FLOAT(40.7130f, fence.maxLat());
    TEST_ASSERT_EQUAL_FLOAT(-74.0070f, fence.minLon());
    TEST_ASSERT_EQUAL_FLOAT(-74.0060f, fence.maxLon());
}

void test_vertex_count(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    TEST_ASSERT_EQUAL_UINT(4, fence.vertexCount());
}

// ============================================================================
// Invalid Polygon Tests
// ============================================================================

void test_null_vertices(void) {
    Polygon fence(nullptr, 4);
    
    GeoPoint point = {40.7125f, -74.0065f};
    TEST_ASSERT_FALSE(fence.contains(point));
}

void test_insufficient_vertices(void) {
    GeoPoint twoPoints[] = {
        {40.7120f, -74.0070f},
        {40.7120f, -74.0060f}
    };
    
    Polygon fence(twoPoints, 2);
    
    GeoPoint point = {40.7120f, -74.0065f};
    TEST_ASSERT_FALSE(fence.contains(point));
}

// ============================================================================
// Standalone Function Tests
// ============================================================================

void test_standalone_function_inside(void) {
    GeoPoint insidePoint = {40.7125f, -74.0065f};
    TEST_ASSERT_TRUE(pointInPolygon(insidePoint, squarePolygon, squarePolygonCount));
}

void test_standalone_function_outside(void) {
    GeoPoint outsidePoint = {40.7140f, -74.0065f};
    TEST_ASSERT_FALSE(pointInPolygon(outsidePoint, squarePolygon, squarePolygonCount));
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_point_near_edge_inside(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point just inside the left edge
    GeoPoint nearEdgePoint = {40.7125f, -74.0069f};
    TEST_ASSERT_TRUE(fence.contains(nearEdgePoint));
}

void test_point_near_edge_outside(void) {
    Polygon fence(squarePolygon, squarePolygonCount);
    
    // Point just outside the left edge
    GeoPoint nearEdgePoint = {40.7125f, -74.0071f};
    TEST_ASSERT_FALSE(fence.contains(nearEdgePoint));
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {
    // Set up code before each test (if needed)
}

void tearDown(void) {
    // Clean up code after each test (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Basic tests
    RUN_TEST(test_point_inside_square);
    RUN_TEST(test_point_outside_square);
    RUN_TEST(test_point_outside_square_west);
    RUN_TEST(test_point_outside_square_south);

    // Concave polygon tests
    RUN_TEST(test_point_inside_concave);
    RUN_TEST(test_point_in_concave_cavity);

    // Triangle tests
    RUN_TEST(test_point_inside_triangle);
    RUN_TEST(test_point_outside_triangle);

    // Boundary cases
    RUN_TEST(test_point_on_edge);
    RUN_TEST(test_point_on_vertex);

    // Bounding box tests
    RUN_TEST(test_bounding_box_rejection);
    RUN_TEST(test_bounding_box_accessors);
    RUN_TEST(test_vertex_count);

    // Invalid polygon tests
    RUN_TEST(test_null_vertices);
    RUN_TEST(test_insufficient_vertices);

    // Standalone function tests
    RUN_TEST(test_standalone_function_inside);
    RUN_TEST(test_standalone_function_outside);

    // Edge cases
    RUN_TEST(test_point_near_edge_inside);
    RUN_TEST(test_point_near_edge_outside);

    return UNITY_END();
}
