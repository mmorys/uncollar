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

// Read one byte from an SX1276 register directly over SPI (no RadioLib).
// Useful for checking SPI wiring before RadioLib init.
static uint8_t spiReadReg(SPIClass& spi, int csPin, uint8_t reg) {
    spi.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);
    spi.transfer(reg & 0x7F);  // read: MSB = 0
    uint8_t val = spi.transfer(0x00);
    digitalWrite(csPin, HIGH);
    spi.endTransaction();
    return val;
}

bool RFM95Radio::begin() {
    _spi = new SPIClass(FSPI);
    _spi->begin(kSpiSck, kSpiMiso, kSpiMosi, _config.csPin);
    pinMode(_config.csPin, OUTPUT);
    digitalWrite(_config.csPin, HIGH);

    if (_config.rstPin != RADIOLIB_NC) {
        pinMode(_config.rstPin, OUTPUT);
        digitalWrite(_config.rstPin, LOW);
        delay(10);
        digitalWrite(_config.rstPin, HIGH);
        delay(10);
    }

    // Raw SPI read of the version register (0x42) — SX1276 should return 0x12
    uint8_t version = spiReadReg(*_spi, _config.csPin, 0x42);
    Serial.print("SX1276 version reg (0x42): 0x");
    Serial.print(version, HEX);
    if (version == 0x12) {
        Serial.println(" — chip found");
    } else if (version == 0x00 || version == 0xFF) {
        Serial.println(" — SPI not responding (check MISO/MOSI/CLK/CS wiring)");
    } else {
        Serial.println(" — unexpected value (wrong chip or wiring issue)");
    }

    _mod   = new Module(_config.csPin, _config.dio0Pin, _config.rstPin,
                        RADIOLIB_NC, *_spi);
    _radio = new SX1276(_mod);

    // LORA_SYNC_WORD is injected via collar/credentials.ini (gitignored).
    // Falls back to the RadioLib default (0x12) if the build flag is absent.
#ifndef LORA_SYNC_WORD
#define LORA_SYNC_WORD RADIOLIB_SX127X_SYNC_WORD
#endif
    int16_t state = _radio->begin(
        _config.frequency,
        125.0f,            // bandwidth kHz
        9,                 // spreading factor
        7,                 // coding rate denominator (4/7)
        LORA_SYNC_WORD,
        _config.txPower,
        8                  // preamble length symbols
    );
    Serial.print("RadioLib SX1276::begin() state: ");
    Serial.print(state);
    Serial.println(state == RADIOLIB_ERR_NONE ? " (OK)" : " (RADIOLIB_ERR_CHIP_NOT_FOUND=-2, RADIOLIB_ERR_SPI_CMD_FAILED=-3)");
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

ConfigReceiveResult RFM95Radio::receiveConfig(ConfigUpdate& configOut,
                                               bool& warnEnabled,
                                               uint32_t timeoutMs) {
    // ConfigUpdate wire format:
    //   [0x10][defaultLat:f32][defaultLon:f32][warnAfter:u16][repeatWarn:u16]
    //   [warnAction:u8][vertexCount:u8][vertex0.lat:f32][vertex0.lon:f32]...
    //   Size: 15 + vertexCount * 8 bytes  (max 143 for 16 vertices)
    constexpr size_t kConfigHeaderLen = 1 + 4 + 4 + 2 + 2 + 1 + 1;
    constexpr size_t kMaxLen = kConfigHeaderLen + RADIO_MAX_BOUNDARY_VERTICES * sizeof(GeoPoint);

    // WarnEnable wire format: [0x11][enabled:u8]  (2 bytes)
    constexpr size_t kWarnEnableLen = 2;

    uint8_t buf[kMaxLen];

    _radio->startReceive();
    uint32_t deadline = millis() + timeoutMs;

    while (millis() < deadline) {
        if (_radio->available()) {
            size_t rxLen   = _radio->getPacketLength();
            size_t readLen = rxLen < kMaxLen ? rxLen : kMaxLen;
            int16_t state  = _radio->readData(buf, readLen);
            _radio->startReceive();  // re-arm for next packet

            if (state != RADIOLIB_ERR_NONE || rxLen < 1) continue;

            if (buf[0] == static_cast<uint8_t>(MessageType::WarnEnable)) {
                if (rxLen != kWarnEnableLen) continue;
                warnEnabled = buf[1] != 0;
                _lastRssi = static_cast<int>(_radio->getRSSI());
                return ConfigReceiveResult::WarnEnable;
            }

            if (buf[0] == static_cast<uint8_t>(MessageType::ConfigUpdate)) {
                if (rxLen < kConfigHeaderLen) continue;
                size_t offset = 1;
                memcpy(&configOut.defaultLatitude,   buf + offset, 4); offset += 4;
                memcpy(&configOut.defaultLongitude,  buf + offset, 4); offset += 4;
                memcpy(&configOut.warnAfterSeconds,  buf + offset, 2); offset += 2;
                memcpy(&configOut.repeatWarnSeconds, buf + offset, 2); offset += 2;
                uint8_t actionByte = buf[offset++];
                configOut.warnAction = (actionByte == static_cast<uint8_t>(WarnAction::Vibrate))
                                           ? WarnAction::Vibrate
                                           : WarnAction::Beep;
                configOut.vertexCount = buf[offset++];

                if (configOut.vertexCount > RADIO_MAX_BOUNDARY_VERTICES) continue;
                if (rxLen != kConfigHeaderLen + configOut.vertexCount * sizeof(GeoPoint)) continue;

                memcpy(configOut.boundaryVertices, buf + offset,
                       configOut.vertexCount * sizeof(GeoPoint));
                _lastRssi = static_cast<int>(_radio->getRSSI());
                return ConfigReceiveResult::ConfigUpdate;
            }
        }
        delay(5);
    }
    return ConfigReceiveResult::None;
}

int RFM95Radio::lastRssi() const {
    return _lastRssi;
}

#endif // ARDUINO
