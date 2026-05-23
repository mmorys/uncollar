/**
 * @file test_gps_manager.cpp
 * @brief Unit tests for GPS manager utility functions (native, no hardware).
 *
 * Tests ddmmToDecimal(), the NMEA DDMM.MMMM to decimal-degree converter that
 * runs on the collar to interpret raw Adafruit GPS output.
 */

#include <unity.h>
#include "../../lib/gps_manager/gps_manager.h"

void setUp() {}
void tearDown() {}

// ============================================
// ddmmToDecimal — northern / eastern hemisphere
// ============================================

void test_north_positive() {
    // 4043.8765 N → 40 + 43.8765/60 = 40.731275°
    float result = ddmmToDecimal(4043.8765f, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 40.7313f, result);
}

void test_east_positive() {
    // 07401.2699 E → 74 + 1.2699/60 = 74.02115°
    float result = ddmmToDecimal(7401.2699f, 'E');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 74.0212f, result);
}

// ============================================
// ddmmToDecimal — southern / western hemisphere
// ============================================

void test_south_negative() {
    // 3352.5000 S → -(33 + 52.5/60) = -33.875°
    float result = ddmmToDecimal(3352.5000f, 'S');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -33.875f, result);
}

void test_west_negative() {
    // 07401.2699 W → -(74 + 1.2699/60) = -74.02115°
    float result = ddmmToDecimal(7401.2699f, 'W');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -74.0212f, result);
}

// ============================================
// ddmmToDecimal — edge cases
// ============================================

void test_equator_zero_degrees() {
    // 0000.0000 N → 0.0°
    float result = ddmmToDecimal(0.0f, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result);
}

void test_whole_degrees_no_minutes() {
    // 4500.0000 N → 45.0° exactly
    float result = ddmmToDecimal(4500.0f, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 45.0f, result);
}

void test_thirty_minutes_is_half_degree() {
    // 0030.0000 N → 0 + 30/60 = 0.5°
    float result = ddmmToDecimal(30.0f, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, result);
}

void test_high_latitude() {
    // 8959.9999 N → 89 + 59.9999/60 ≈ 89.99998°
    float result = ddmmToDecimal(8959.9999f, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, result);
}

void test_default_home_latitude() {
    // DEFAULT_LATITUDE is 40.72272; build the DDMM value and round-trip it
    // 40.72272° → 4043.3632 DDMM
    float ddmm   = 40.0f * 100.0f + 0.72272f * 60.0f;  // 4043.3632
    float result = ddmmToDecimal(ddmm, 'N');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 40.72272f, result);
}

void test_default_home_longitude() {
    // DEFAULT_LONGITUDE is -74.02116; build DDMM for 74.02116° W
    float ddmm   = 74.0f * 100.0f + 0.02116f * 60.0f;  // 7401.2696
    float result = ddmmToDecimal(ddmm, 'W');
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -74.02116f, result);
}

// ============================================
// Entry point
// ============================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_north_positive);
    RUN_TEST(test_east_positive);
    RUN_TEST(test_south_negative);
    RUN_TEST(test_west_negative);
    RUN_TEST(test_equator_zero_degrees);
    RUN_TEST(test_whole_degrees_no_minutes);
    RUN_TEST(test_thirty_minutes_is_half_degree);
    RUN_TEST(test_high_latitude);
    RUN_TEST(test_default_home_latitude);
    RUN_TEST(test_default_home_longitude);

    return UNITY_END();
}
