#pragma once

// Hardware pin assignments for Adafruit QT Py ESP32-S3 collar build.
// All SPI and radio connections are hardwired (not jumpered).

// FSPI bus
static constexpr int kSpiSck  = 36;
static constexpr int kSpiMiso = 37;
static constexpr int kSpiMosi = 35;

// RFM95W radio
static constexpr int kRadioCs   = 18;  // NSS — A0 (GPIO18)
static constexpr int kRadioDio0 = 33;
static constexpr int kRadioRst  = -1;  // unconnected; maps to RADIOLIB_NC

// I2C bus 1 — GPS + LCD
static constexpr int kI2c1Sda = 41;
static constexpr int kI2c1Scl = 40;
