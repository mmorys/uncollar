#pragma once

// Hardware pin assignments for Adafruit QT Py ESP32 Pico basestation build.
// All SPI and radio connections are hardwired (not jumpered).

// HSPI bus — GPIO 12/13/14 are the HSPI defaults on classic ESP32
static constexpr int kSpiSck  = 14;
static constexpr int kSpiMiso = 12;
static constexpr int kSpiMosi = 13;

// RFM95W radio
static constexpr int kRadioCs   = 26;  // NSS — A0 (GPIO26)
static constexpr int kRadioDio0 = -1;  // TBD
static constexpr int kRadioRst  = -1;  // unconnected; maps to RADIOLIB_NC
