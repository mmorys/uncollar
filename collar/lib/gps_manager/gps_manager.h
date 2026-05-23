/**
 * @file gps_manager.h
 * @brief GPS subsystem interface and Adafruit concrete implementation.
 *
 * IGpsManager defines the API contract for GPS operations. The free function
 * ddmmToDecimal() handles NMEA DDMM.MMMM to decimal-degree conversion and
 * is platform-independent so it can be unit-tested on native.
 *
 * @copyright Apache 2.0 License
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <stdint.h>
#include "../point_in_polygon/point_in_polygon.h"

// ============================================
// DATA TYPES
// ============================================

/**
 * @brief Result of a GPS fix attempt.
 */
struct GpsFix {
    GeoPoint position;   ///< Decimal-degree lat/lon
    uint32_t timestamp;  ///< millis() at fix acquisition; 0 if invalid
    float    hdop;       ///< Horizontal dilution of precision (lower = better)
    uint8_t  satellites; ///< Number of satellites used
    bool     valid;      ///< true if fix was acquired
};

// ============================================
// UTILITY FUNCTION (testable on native)
// ============================================

/**
 * @brief Convert NMEA DDMM.MMMM format to decimal degrees.
 *
 * @param ddmm       Raw NMEA value (e.g. 4043.8765 for 40°43.8765')
 * @param hemisphere 'N'/'E' → positive; 'S'/'W' → negative
 * @return Decimal degrees
 */
float ddmmToDecimal(float ddmm, char hemisphere);

// ============================================
// ABSTRACT INTERFACE
// ============================================

/**
 * @brief Abstract GPS manager interface.
 *
 * Implementations wrap a specific GPS module and present a uniform
 * API for initialization, fix acquisition, and power control.
 */
class IGpsManager {
public:
    virtual ~IGpsManager() = default;

    /**
     * @brief Initialize the GPS module.
     * @return true on success
     */
    virtual bool begin() = 0;

    /**
     * @brief Block until a GPS fix is acquired or the timeout expires.
     * @param timeoutMs Maximum wait time in milliseconds
     * @return true if fix acquired before timeout
     */
    virtual bool waitForFix(uint32_t timeoutMs) = 0;

    /**
     * @brief Return the most recent fix (valid or not).
     *
     * Call after waitForFix(). If waitForFix() returned false the returned
     * struct will have valid == false.
     */
    virtual GpsFix getLastFix() const = 0;

    /**
     * @brief Put the GPS module into standby/sleep mode.
     */
    virtual void sleep() = 0;

    /**
     * @brief Wake the GPS module from standby mode.
     */
    virtual void wake() = 0;
};

// ============================================
// CONCRETE IMPLEMENTATION (Arduino only)
// ============================================

#ifdef ARDUINO
#include <Wire.h>
#include <Adafruit_GPS.h>

/**
 * @brief IGpsManager implementation for the Adafruit Mini GPS PA1010D.
 *
 * Communicates over I2C at address 0x10. Requests RMC-only NMEA output
 * at 1 Hz and converts raw DDMM values to decimal degrees via ddmmToDecimal().
 */
class AdafruitGpsManager : public IGpsManager {
public:
    /**
     * @param wire I2C bus the GPS is connected to (default Wire1, pins 41/40)
     */
    explicit AdafruitGpsManager(TwoWire& wire = Wire1);

    bool   begin() override;
    bool   waitForFix(uint32_t timeoutMs) override;
    GpsFix getLastFix() const override;
    void   sleep() override;
    void   wake() override;

private:
    Adafruit_GPS _gps;
    GpsFix       _lastFix;
};
#endif // ARDUINO

#endif // GPS_MANAGER_H
