// Radio TX demo — transmits a PositionReport every second so the RF output
// can be verified with a spectrum analyzer. Not a unit test; runs until reset.

#include <Arduino.h>
#include "../../lib/radio/radio.h"

static constexpr RFM95Config kRadioConfig = {
    .csPin     = 17,
    .dio0Pin   = 33,
    .rstPin    = 18,
    .frequency = 433.0f,
    .txPower   = 17,
};

static RFM95Radio radio(kRadioConfig);
static bool       radioOk   = false;
static uint32_t   txCount   = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("Radio TX demo — 433 MHz, SF9, BW125, CR4/7");
    Serial.println("Transmitting PositionReport every 1 s ...");

    radioOk = radio.begin();
    if (!radioOk) {
        Serial.println("ERROR: radio.begin() failed — check wiring");
    }
}

void loop() {
    if (!radioOk) {
        delay(1000);
        return;
    }

    PositionReport report = {
        .latitude       = 52.2297f,   // dummy Warsaw coords
        .longitude      = 21.0122f,
        .timestamp      = millis(),
        .satellites     = 7,
        .insideBoundary = true,
        .batteryMv      = 3700,
    };

    bool ok = radio.sendPositionReport(report);
    txCount++;

    Serial.print("TX #");
    Serial.print(txCount);
    Serial.println(ok ? " OK" : " FAIL");

    delay(1000);
}
