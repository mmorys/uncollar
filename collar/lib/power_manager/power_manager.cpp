/**
 * @file power_manager.cpp
 * @brief ESP32 power manager implementation.
 */

#include "power_manager.h"

#ifdef ARDUINO

#include <WiFi.h>
#include <esp_sleep.h>

void Esp32PowerManager::disableUnusedPeripherals() {
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
    btStop();
}

void Esp32PowerManager::enterDeepSleep(uint32_t durationUs) {
    esp_sleep_enable_timer_wakeup(durationUs);
    esp_deep_sleep_start();
}

WakeReason Esp32PowerManager::getWakeReason() const {
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:  return WakeReason::DeepSleepTimer;
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:   return WakeReason::ExternalPin;
        case ESP_SLEEP_WAKEUP_UNDEFINED: return WakeReason::PowerOn;
        default:                      return WakeReason::Unknown;
    }
}

#endif // ARDUINO
