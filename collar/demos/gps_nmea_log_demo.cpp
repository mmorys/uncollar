#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GPS.h>

Adafruit_GPS GPS(&Wire1);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    Wire1.begin(41, 40);

    Serial.println("GPS NMEA log demo — initializing...");

    if (!GPS.begin(0x10)) {
        Serial.println("ERROR: no GPS found at 0x10 — check wiring");
        while (1) delay(100);
    }

    // Enable all NMEA sentences: GGA, GLL, GSA, GSV, RMC, VTG
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    Serial.println("GPS ready — logging all NMEA sentences:\n");
}

void loop() {
    GPS.read();

    if (GPS.newNMEAreceived()) {
        Serial.println(GPS.lastNMEA());
    }
}
