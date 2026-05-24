// Radio TX demo — transmits a PositionReport every second so the RF output
// can be verified with a spectrum analyzer. Not a unit test; runs until reset.

#include <Arduino.h>
#include "radio.h"
#include "pins.h"

static constexpr RFM95Config kRadioConfig = {
    .csPin     = kRadioCs,
    .dio0Pin   = kRadioDio0,
    .rstPin    = kRadioRst,
    .frequency = 915.0f,
    .txPower   = 17,
};

static RFM95Radio radio(kRadioConfig);
static bool       radioOk = false;
static uint32_t   txCount = 0;

void setup() {
    Serial.begin(115200);
    // Give USB-CDC time to enumerate before any output
    delay(2000);

    Serial.println("=== Radio TX demo ===");
    Serial.print("SPI:  SCK="); Serial.print(kSpiSck);
    Serial.print(" MISO=");    Serial.print(kSpiMiso);
    Serial.print(" MOSI=");    Serial.println(kSpiMosi);
    Serial.print  ("Pins: CS=");  Serial.print(kRadioConfig.csPin);
    Serial.print  (" DIO0=");     Serial.print(kRadioConfig.dio0Pin);
    Serial.print  (" RST=");      Serial.println(kRadioConfig.rstPin);
    Serial.print  ("Freq: ");     Serial.print(kRadioConfig.frequency); Serial.println(" MHz");
    Serial.print  ("Power: ");    Serial.print(kRadioConfig.txPower);   Serial.println(" dBm");
    Serial.println("SF9 BW125 CR4/7");
    Serial.println();

    Serial.println("Initializing radio...");
    radioOk = radio.begin();

    if (radioOk) {
        Serial.println("Radio init OK — starting TX loop");
    } else {
        Serial.println("ERROR: radio.begin() failed");
        Serial.println("Check: SPI wiring, CS/DIO0/RST pins, 3.3 V supply, antenna");
    }
}

void loop() {
    if (!radioOk) {
        Serial.println("(radio not initialized — retrying begin())");
        radioOk = radio.begin();
        delay(2000);
        return;
    }

    PositionReport report = {
        .latitude       = 52.2297f,
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
    if (ok) {
        Serial.println(" OK");
    } else {
        Serial.println(" FAIL — transmit returned error");
    }

    delay(1000);
}
