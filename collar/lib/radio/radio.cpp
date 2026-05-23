/**
 * @file radio.cpp
 * @brief RFM95 LoRa radio stub implementation.
 *
 * These are placeholder implementations. Replace with RadioLib (or a similar
 * driver) calls once the RFM95 hardware is wired up.
 */

#include "radio.h"

#ifdef ARDUINO

bool RFM95Radio::begin() {
    // TODO: initialise SPI, configure RFM95 frequency and TX power
    return false;
}

bool RFM95Radio::sendPositionReport(const PositionReport&) {
    // TODO: serialise report, call radio.transmit()
    return false;
}

bool RFM95Radio::sendBoundaryAlert(const BoundaryAlert&) {
    // TODO: serialise alert, call radio.transmit()
    return false;
}

bool RFM95Radio::receiveConfigUpdate(ConfigUpdate&, uint32_t) {
    // TODO: call radio.receive() with timeout, deserialise ConfigUpdate
    return false;
}

int RFM95Radio::lastRssi() const {
    return _lastRssi;
}

#endif // ARDUINO
