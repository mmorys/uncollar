/**
 * @file gps_manager.cpp
 * @brief GPS manager implementation.
 */

#include "gps_manager.h"

// ============================================
// UTILITY FUNCTION
// ============================================

float ddmmToDecimal(float ddmm, char hemisphere) {
    int   degrees = (int)(ddmm / 100);
    float minutes = ddmm - (degrees * 100.0f);
    float decimal = degrees + (minutes / 60.0f);
    if (hemisphere == 'S' || hemisphere == 'W') {
        decimal = -decimal;
    }
    return decimal;
}

// ============================================
// AdafruitGpsManager (Arduino only)
// ============================================

#ifdef ARDUINO

AdafruitGpsManager::AdafruitGpsManager(TwoWire& wire)
    : _gps(&wire), _lastFix{{0.0f, 0.0f}, 0, 0.0f, 0, false} {}

bool AdafruitGpsManager::begin() {
    // The PA1010D needs a brief moment to be ready on I2C after power-on.
    // Adafruit_GPS::begin() returns false if the chip doesn't ACK its address;
    // retry until it does (up to ~2 s total) before giving up.
    constexpr int      kMaxRetries    = 20;
    constexpr uint32_t kRetryDelayMs  = 100;
    for (int i = 0; i < kMaxRetries; i++) {
        if (_gps.begin(0x10)) {
            _gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
            _gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
            return true;
        }
        delay(kRetryDelayMs);
    }
    return false;
}

bool AdafruitGpsManager::waitForFix(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    while (millis() - startTime < timeoutMs) {
        _gps.read();
        if (_gps.newNMEAreceived()) {
            if (!_gps.parse(_gps.lastNMEA())) {
                continue;
            }
            if (_gps.fix) {
                _lastFix.position.lat = ddmmToDecimal(_gps.latitude,  _gps.lat);
                _lastFix.position.lon = ddmmToDecimal(_gps.longitude, _gps.lon);
                _lastFix.timestamp   = millis();
                _lastFix.hdop        = _gps.HDOP;
                _lastFix.satellites  = _gps.satellites;
                _lastFix.valid       = true;
                return true;
            }
        }
    }
    _lastFix.valid = false;
    return false;
}

GpsFix AdafruitGpsManager::getLastFix() const {
    return _lastFix;
}

void AdafruitGpsManager::sleep() {
    _gps.sendCommand(PMTK_STANDBY);
}

void AdafruitGpsManager::wake() {
    _gps.sendCommand(PMTK_AWAKE);
}

#endif // ARDUINO
