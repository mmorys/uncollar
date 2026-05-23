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
 * @brief Base Station → Collar: updated geofence and home position.
 *
 * The collar persists this to NVS upon receipt.
 */
struct ConfigUpdate {
    float    defaultLatitude;
    float    defaultLongitude;
    GeoPoint boundaryVertices[RADIO_MAX_BOUNDARY_VERTICES];
    uint8_t  vertexCount;
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

/**
 * @brief IRadio stub for the HopeRF RFM95 LoRa module.
 *
 * Not yet wired to a hardware driver. Provides the interface so that
 * main.cpp can be written against it; replace the stub methods with
 * RadioLib (or equivalent) calls when the hardware is available.
 */
class RFM95Radio : public IRadio {
public:
    bool begin() override;
    bool sendPositionReport(const PositionReport& report) override;
    bool sendBoundaryAlert(const BoundaryAlert& alert) override;
    bool receiveConfigUpdate(ConfigUpdate& out, uint32_t timeoutMs) override;
    int  lastRssi() const override;

private:
    int _lastRssi = 0;
};

#endif // ARDUINO

#endif // RADIO_H
