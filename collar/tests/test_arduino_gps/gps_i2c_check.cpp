// GPS I2C communication tests for the Adafruit PA1010D (address 0x10).
// Requires GPS module connected to Wire1 (SDA=41, SCL=40).
// Radio module need NOT be connected.

#include <Arduino.h>
#include <Wire.h>
#include <unity.h>
#include "../../lib/gps_manager/gps_manager.h"

static AdafruitGpsManager gps(Wire1);

void setUp() {}
void tearDown() {}

// Verify the PA1010D acknowledges on I2C before involving any library code.
void test_gps_i2c_ack() {
    Wire1.beginTransmission(0x10);
    uint8_t err = Wire1.endTransmission();
    TEST_ASSERT_EQUAL_MESSAGE(0, err, "No ACK from GPS at 0x10 — check power and wiring");
}

// Verify the manager can initialize the GPS and configure NMEA output.
void test_gps_begin() {
    bool ok = gps.begin();
    TEST_ASSERT_TRUE_MESSAGE(ok, "AdafruitGpsManager::begin() failed");
}

// After begin(), the GPS should emit NMEA sentences every second at 1 Hz.
// Read raw I2C bytes for up to 2 s and look for the NMEA start character '$'.
// The PA1010D pads empty reads with 0x0A, so any '$' confirms data is flowing.
void test_gps_nmea_data_flowing() {
    bool found = false;
    uint32_t deadline = millis() + 2000;
    while (millis() < deadline && !found) {
        Wire1.requestFrom((uint8_t)0x10, (uint8_t)32);
        while (Wire1.available()) {
            if ((char)Wire1.read() == '$') {
                found = true;
                break;
            }
        }
        delay(50);
    }
    TEST_ASSERT_TRUE_MESSAGE(found, "No NMEA '$' in 2 s — GPS may not be sending data");
}

// Smoke-test that sleep/wake commands complete without hang.
void test_gps_wake_sleep() {
    gps.sleep();
    delay(200);
    gps.wake();
    delay(200);
    TEST_PASS_MESSAGE("wake/sleep commands completed without hang");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);  // wait for USB-CDC to enumerate
    Wire1.begin(41, 40);

    UNITY_BEGIN();
    RUN_TEST(test_gps_i2c_ack);
    RUN_TEST(test_gps_begin);
    RUN_TEST(test_gps_nmea_data_flowing);
    RUN_TEST(test_gps_wake_sleep);
    UNITY_END();
}

void loop() {}
