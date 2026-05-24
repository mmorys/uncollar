// Raw SPI diagnostic — no RadioLib. Reads known SX1276 registers and does a
// write-then-read to verify the full MOSI→chip→MISO round-trip.

#include <Arduino.h>
#include <SPI.h>
#include "pins.h"

// SX1276 register map (subset)
static constexpr uint8_t REG_FIFO        = 0x00; // R/W — good write-then-read target
static constexpr uint8_t REG_OP_MODE     = 0x01; // default 0x09 (sleep, FSK)
static constexpr uint8_t REG_VERSION     = 0x42; // always 0x12 on genuine SX1276

static SPIClass spi(FSPI);

static uint8_t readReg(uint8_t reg) {
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(kRadioCs, LOW);
    spi.transfer(reg & 0x7F); // MSB=0 → read
    uint8_t val = spi.transfer(0x00);
    digitalWrite(kRadioCs, HIGH);
    spi.endTransaction();
    return val;
}

static void writeReg(uint8_t reg, uint8_t val) {
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(kRadioCs, LOW);
    spi.transfer(reg | 0x80); // MSB=1 → write
    spi.transfer(val);
    digitalWrite(kRadioCs, HIGH);
    spi.endTransaction();
}

static void printHex(uint8_t v) {
    if (v < 0x10) Serial.print('0');
    Serial.print(v, HEX);
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("=== SPI raw diagnostic (no RadioLib) ===");
    Serial.print("SPI: SCK="); Serial.print(kSpiSck);
    Serial.print(" MISO=");    Serial.print(kSpiMiso);
    Serial.print(" MOSI=");    Serial.print(kSpiMosi);
    Serial.print(" CS=");      Serial.println(kRadioCs);
    Serial.println("Clock: 1 MHz, SPI_MODE0, MSBFIRST");
    Serial.println();

    pinMode(kRadioCs, OUTPUT);
    digitalWrite(kRadioCs, HIGH);
    spi.begin(kSpiSck, kSpiMiso, kSpiMosi, kRadioCs);

    Serial.println("RST unconnected — relying on chip power-on reset.");
    Serial.println();

    // --- Register reads ---
    Serial.println("--- Register reads ---");

    uint8_t version = readReg(REG_VERSION);
    Serial.print("0x42 RegVersion:  0x"); printHex(version);
    if (version == 0x12)        Serial.println("  PASS (SX1276 found)");
    else if (version == 0xFF)   Serial.println("  FAIL (MISO stuck high — check MISO wire or CS)");
    else if (version == 0x00)   Serial.println("  FAIL (MISO stuck low — check MISO wire or power)");
    else                        Serial.println("  UNEXPECTED (wrong chip or partial SPI response)");

    uint8_t opMode = readReg(REG_OP_MODE);
    Serial.print("0x01 RegOpMode:   0x"); printHex(opMode);
    if (opMode == 0x09)         Serial.println("  PASS (default sleep/FSK mode)");
    else                        Serial.println("  (non-default — may be OK if chip was previously configured)");

    Serial.println();

    // --- Write-then-read on FIFO register ---
    Serial.println("--- Write-then-read (FIFO register 0x00) ---");
    uint8_t pattern = 0xA5;
    writeReg(REG_FIFO, pattern);
    uint8_t readback = readReg(REG_FIFO);
    Serial.print("Wrote: 0x"); printHex(pattern);
    Serial.print("  Read back: 0x"); printHex(readback);
    if (readback == pattern)    Serial.println("  PASS");
    else                        Serial.println("  FAIL (MOSI or MISO not working correctly)");

    Serial.println();
    Serial.println("--- Interpretation ---");
    if (version == 0x12 && readback == pattern) {
        Serial.println("SPI bus OK. RadioLib should be able to talk to the chip.");
    } else if (version == 0xFF || version == 0x00) {
        Serial.println("SPI bus not responding. Check:");
        Serial.println("  1. MISO/MOSI not swapped");
        Serial.println("  2. CS (active low) reaching the chip");
        Serial.println("  3. 3.3 V present on radio VCC");
        Serial.println("  4. Common GND between ESP32 and radio");
    } else {
        Serial.println("Partial response. Check SPI_MODE (try MODE0 vs MODE1) and clock speed.");
    }
}

void loop() {}
