#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GPS.h>

Adafruit_GPS GPS(&Wire1);

static uint32_t lastPrint = 0;

static void scanI2C() {
    Serial.println("Scanning I2C bus (Wire1, SDA=41 SCL=40)...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire1.beginTransmission(addr);
        if (Wire1.endTransmission() == 0) {
            Serial.print("  Device at 0x");
            if (addr < 16) Serial.print("0");
            Serial.println(addr, HEX);
            found++;
        }
    }
    if (found == 0) Serial.println("  No devices found — check wiring and power");
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    Wire1.begin(41, 40);

    scanI2C();

    Serial.println("GPS demo — initializing...");

    if (!GPS.begin(0x10)) {
        Serial.println("ERROR: no GPS found at 0x10 — check wiring");
        while (1) delay(100);
    }

    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    Serial.println("GPS ready. Waiting for fix...\n");
}

void loop() {
    GPS.read();

    if (GPS.newNMEAreceived()) {
        GPS.parse(GPS.lastNMEA());
    }

    if (millis() - lastPrint < 2000) return;
    lastPrint = millis();

    Serial.print("Fix: ");       Serial.println(GPS.fix ? "YES" : "NO (searching...)");
    Serial.print("  Satellites: "); Serial.println(GPS.satellites);

    if (GPS.fix) {
        Serial.print("  Lat:      "); Serial.print(GPS.latitude, 4); Serial.println(GPS.lat);
        Serial.print("  Lon:      "); Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
        Serial.print("  HDOP:     "); Serial.println(GPS.HDOP);
        Serial.print("  Altitude: "); Serial.print(GPS.altitude); Serial.println(" m");
        Serial.print("  Speed:    "); Serial.print(GPS.speed); Serial.println(" knots");
    }
    Serial.println();
}
