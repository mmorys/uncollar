/**
 * @file power_manager.h
 * @brief Power management interface and ESP32 concrete implementation.
 *
 * IPowerManager defines the API for peripheral shutdown and deep sleep.
 * Esp32PowerManager implements it using the Arduino WiFi/BT APIs and the
 * ESP-IDF esp_sleep driver.
 *
 * @copyright Apache 2.0 License
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>

// ============================================
// DATA TYPES
// ============================================

/**
 * @brief Reason the ESP32 woke from deep sleep.
 */
enum class WakeReason {
    PowerOn,         ///< Cold boot or reset
    DeepSleepTimer,  ///< Timer wakeup (normal operating cycle)
    ExternalPin,     ///< External GPIO trigger
    Unknown,         ///< Any other cause
};

// ============================================
// ABSTRACT INTERFACE
// ============================================

/**
 * @brief Abstract power management interface.
 */
class IPowerManager {
public:
    virtual ~IPowerManager() = default;

    /**
     * @brief Disable radios and other unused peripherals to reduce current draw.
     *
     * Should be called once near the start of each wake cycle.
     */
    virtual void disableUnusedPeripherals() = 0;

    /**
     * @brief Enter deep sleep for the given duration.
     *
     * This function never returns. The MCU will wake and re-execute setup().
     *
     * @param durationUs Sleep duration in microseconds
     */
    virtual void enterDeepSleep(uint32_t durationUs) = 0;

    /**
     * @brief Return the reason this wake cycle started.
     */
    virtual WakeReason getWakeReason() const = 0;
};

// ============================================
// CONCRETE IMPLEMENTATION (Arduino / ESP32 only)
// ============================================

#ifdef ARDUINO

/**
 * @brief IPowerManager implementation for the ESP32 Arduino platform.
 *
 * Disables WiFi and Bluetooth (LoRa handles radio comms), then uses
 * esp_sleep_enable_timer_wakeup() + esp_deep_sleep_start() for the
 * deep sleep cycle.
 */
class Esp32PowerManager : public IPowerManager {
public:
    void       disableUnusedPeripherals() override;
    void       enterDeepSleep(uint32_t durationUs) override;
    WakeReason getWakeReason() const override;
};

#endif // ARDUINO

#endif // POWER_MANAGER_H
