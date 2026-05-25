/**
 * @file radio.h
 * @brief LoRa radio interface, wire message types, and RFM95 concrete stub.
 *
 * This header defines the binary protocol shared between the collar and the
 * base station. All structs are packed to match the on-wire layout. Message
 * types are pure C++ (no Arduino dependency) so they can be shared with
 * non-Arduino base-station code and used in native unit tests.
 *
 * @copyright Apache 2.0 License
 */

#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <stddef.h>
#include "../point_in_polygon/point_in_polygon.h"

// Maximum boundary vertices carried in a ConfigUpdate message.
// Must be <= config_manager.h MAX_BOUNDARY_VERTICES (currently 16).
constexpr size_t RADIO_MAX_BOUNDARY_VERTICES = 16;

// ============================================
// WIRE MESSAGE TYPES
// ============================================

/**
 * @brief One-byte message type tag prepended to every LoRa packet.
 */
enum class MessageType : uint8_t {
    PositionReport = 0x01,  ///< Collar → Base: current position
    BoundaryAlert  = 0x02,  ///< Collar → Base: boundary crossing event
    ConfigUpdate   = 0x10,  ///< Base → Collar: new geofence / home position
    Ack            = 0x20,  ///< Either direction: acknowledgement
};

/**
 * @brief Collar → Base Station: periodic position report.
 *
 * Sent every wake cycle after a GPS fix (or from last known position).
 */
struct PositionReport {
    float    latitude;
    float    longitude;
    uint32_t timestamp;      ///< millis() on the collar at fix time
    uint8_t  satellites;
    bool     insideBoundary;
    uint16_t batteryMv;      ///< Battery voltage in mV; 0 if unmeasured
};

/**
 * @brief Collar → Base Station: boundary crossing event.
 *
 * Emitted immediately when the dog transitions in or out of the geofence.
 */
struct BoundaryAlert {
    float    latitude;
    float    longitude;
    uint32_t timestamp;
    bool     enteredBoundary;  ///< true = just entered; false = just exited
};

/**
 * @brief Which actuator fires when the collar warns about a boundary breach.
 *
 * Wire value is a single byte; unknown values are clamped to Beep on the collar.
 */
enum class WarnAction : uint8_t {
    Beep    = 0,
    Vibrate = 1,
};

/**
 * @brief Base Station → Collar: updated geofence, home position, and warn settings.
 *
 * The collar persists this to NVS upon receipt.
 *
 * `warnAfterSeconds` is the duration the collar must be continuously outside the
 * boundary before the first warning fires. `repeatWarnSeconds` is the interval
 * between subsequent warnings while still outside. Both are quantized to the
 * collar's GPS wake interval.
 */
struct ConfigUpdate {
    float      defaultLatitude;
    float      defaultLongitude;
    GeoPoint   boundaryVertices[RADIO_MAX_BOUNDARY_VERTICES];
    uint8_t    vertexCount;
    uint16_t   warnAfterSeconds;
    uint16_t   repeatWarnSeconds;
    WarnAction warnAction;
};

// ============================================
// ABSTRACT INTERFACE
// ============================================

/**
 * @brief Abstract radio interface for LoRa collar ↔ base communication.
 */
class IRadio {
public:
    virtual ~IRadio() = default;

    /**
     * @brief Initialize the radio hardware.
     * @return true on success
     */
    virtual bool begin() = 0;

    /**
     * @brief Transmit a position report to the base station.
     * @return true if transmitted successfully
     */
    virtual bool sendPositionReport(const PositionReport& report) = 0;

    /**
     * @brief Transmit a boundary alert to the base station.
     * @return true if transmitted successfully
     */
    virtual bool sendBoundaryAlert(const BoundaryAlert& alert) = 0;

    /**
     * @brief Listen for a config update from the base station.
     *
     * Blocks up to timeoutMs waiting for a ConfigUpdate packet.
     *
     * @param out       Populated if a valid update arrives
     * @param timeoutMs Maximum wait time in milliseconds
     * @return true if a valid ConfigUpdate was received
     */
    virtual bool receiveConfigUpdate(ConfigUpdate& out, uint32_t timeoutMs) = 0;

    /**
     * @brief RSSI of the last received packet in dBm.
     */
    virtual int lastRssi() const = 0;
};

// ============================================
// CONCRETE IMPLEMENTATION (Arduino only)
// ============================================

#ifdef ARDUINO

#include <RadioLib.h>
#include <SPI.h>

/**
 * @brief SPI pin and RF settings for the RFM95W module.
 *
 * Suggested GPIO assignments for the Adafruit QT Py ESP32-S3 — see
 * collar/WIRING.md for the full connection diagram.
 */
struct RFM95Config {
    int   csPin;     ///< SPI chip select (NSS) — A0 = GPIO18 on QT Py ESP32-S3
    int   dio0Pin;   ///< Packet-done interrupt (DIO0)
    int   rstPin;    ///< Hardware reset; use RADIOLIB_NC (-1) if unconnected
    float frequency; ///< Carrier frequency in MHz (433.0 EU / 915.0 US)
    int   txPower;   ///< TX output power in dBm, 2–20 for RFM95W
};

/**
 * @brief IRadio implementation for the HopeRF RFM95W LoRa module via RadioLib.
 *
 * Uses the FSPI bus (SCK=36, MISO=37, MOSI=35) and the pin assignments
 * supplied in RFM95Config. Wire format: 1-byte MessageType prefix followed
 * by the packed struct payload.
 */
class RFM95Radio : public IRadio {
public:
    explicit RFM95Radio(const RFM95Config& config);
    ~RFM95Radio() override;

    bool begin() override;
    bool sendPositionReport(const PositionReport& report) override;
    bool sendBoundaryAlert(const BoundaryAlert& alert) override;
    bool receiveConfigUpdate(ConfigUpdate& out, uint32_t timeoutMs) override;
    int  lastRssi() const override;

private:
    RFM95Config _config;
    SPIClass*   _spi   = nullptr;
    Module*     _mod   = nullptr;
    SX1276*     _radio = nullptr;
    int         _lastRssi = 0;
};

#endif // ARDUINO

#endif // RADIO_H
