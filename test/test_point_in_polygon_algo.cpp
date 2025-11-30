#include "pointInPolygon.h"
#include <unity.h>

float vertx [] = {1, 1, 2, 2, 1};
float verty []  = {-2, -1, -1, -2, -2};
float nvert = 5;

void setUp(void) {
    // set stuff up here
    
}

void tearDown(void) {
    // clean stuff up here
}

void point_in_polygon() {
    TEST_ASSERT_EQUAL_INT(pnpoly(nvert, vertx, verty, 1.5, -1.5), 1);
}

void point_out_polygon() {
    TEST_ASSERT_EQUAL_INT(pnpoly(nvert, vertx, verty, 1.5, -2.5), 0);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(point_in_polygon);
    RUN_TEST(point_out_polygon);
    UNITY_END();
}
