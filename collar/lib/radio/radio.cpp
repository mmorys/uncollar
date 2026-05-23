/**
 * @file radio.cpp
 * @brief RFM95W LoRa radio implementation using RadioLib (SX1276 driver).
 *
 * Wire format: every packet begins with a 1-byte MessageType tag followed
 * immediately by the packed struct payload. No additional framing is needed
 * because RadioLib handles packet boundaries at the LoRa layer.
 *
 * SPI bus: FSPI (SCK=36, MISO=37, MOSI=35). CS, DIO0, and RST pins are
 * supplied via RFM95Config — see collar/WIRING.md for the full pinout.
 */

#include "radio.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <string.h>

// Hardware SPI pins for the Adafruit QT Py ESP32-S3 (FSPI bus).
static constexpr int kSpiSck  = 36;
static constexpr int kSpiMiso = 37;
static constexpr int kSpiMosi = 35;

RFM95Radio::RFM95Radio(const RFM95Config& config) : _config(config) {}

RFM95Radio::~RFM95Radio() {
    delete _radio;
    delete _mod;
    delete _spi;
}

bool RFM95Radio::begin() {
    _spi = new SPIClass(FSPI);
    _spi->begin(kSpiSck, kSpiMiso, kSpiMosi, _config.csPin);

    _mod   = new Module(_config.csPin, _config.dio0Pin, _config.rstPin,
                        RADIOLIB_NC, *_spi);
    _radio = new SX1276(_mod);

    int16_t state = _radio->begin(
        _config.frequency,
        125.0f,            // bandwidth kHz
        9,                 // spreading factor
        7,                 // coding rate denominator (4/7)
        SX127X_SYNC_WORD,
        _config.txPower,
        8                  // preamble length symbols
    );
    return state == RADIOLIB_ERR_NONE;
}

bool RFM95Radio::sendPositionReport(const PositionReport& report) {
    uint8_t buf[1 + sizeof(PositionReport)];
    buf[0] = static_cast<uint8_t>(MessageType::PositionReport);
    memcpy(buf + 1, &report, sizeof(PositionReport));
    return _radio->transmit(buf, sizeof(buf)) == RADIOLIB_ERR_NONE;
}

bool RFM95Radio::sendBoundaryAlert(const BoundaryAlert& alert) {
    uint8_t buf[1 + sizeof(BoundaryAlert)];
    buf[0] = static_cast<uint8_t>(MessageType::BoundaryAlert);
    memcpy(buf + 1, &alert, sizeof(BoundaryAlert));
    return _radio->transmit(buf, sizeof(buf)) == RADIOLIB_ERR_NONE;
}

bool RFM95Radio::receiveConfigUpdate(ConfigUpdate& out, uint32_t timeoutMs) {
    constexpr size_t kExpectedLen = 1 + sizeof(ConfigUpdate);
    uint8_t buf[kExpectedLen];

    _radio->startReceive();
    uint32_t deadline = millis() + timeoutMs;

    while (millis() < deadline) {
        if (_radio->available()) {
            size_t rxLen   = _radio->getPacketLength();
            bool   correct = (rxLen == kExpectedLen);
            size_t readLen = correct ? kExpectedLen
                                     : (rxLen < kExpectedLen ? rxLen : kExpectedLen);
            int16_t state  = _radio->readData(buf, readLen);
            _radio->startReceive();  // re-arm for next packet

            if (!correct || state != RADIOLIB_ERR_NONE) continue;
            if (buf[0] != static_cast<uint8_t>(MessageType::ConfigUpdate)) continue;

            memcpy(&out, buf + 1, sizeof(ConfigUpdate));
            _lastRssi = static_cast<int>(_radio->getRSSI());
            return true;
        }
        delay(5);
    }
    return false;
}

int RFM95Radio::lastRssi() const {
    return _lastRssi;
}

#endif // ARDUINO
