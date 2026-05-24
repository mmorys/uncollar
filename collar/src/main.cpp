#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "I2C_LCD.h"
#include "../lib/point_in_polygon/point_in_polygon.h"
#include "../lib/config_manager/config_manager.h"
#include "../lib/gps_manager/gps_manager.h"
#include "../lib/power_manager/power_manager.h"
#include "../lib/radio/radio.h"

// Uncomment to enable serial debugging output
#define DEBUG_SERIAL

// Uncomment to enable LCD debugging output
// #define DEBUG_LCD

// ============================================
// TIMING CONSTANTS
// ============================================

constexpr uint32_t GPS_UPDATE_INTERVAL_US  = 5UL * 1000000UL;  // deep sleep duration
constexpr uint32_t GPS_FIX_TIMEOUT_MS      = 3000;
constexpr uint32_t CONFIG_RECEIVE_TIMEOUT_MS = 500;

// ============================================
// GLOBALS
// ============================================

// ConfigManager lives as a global so its NVS Preferences handle stays open
// for the full wake cycle. Defined here; extern in any future translation units.
ConfigManager configManager;

// Last known position survives deep sleep in RTC memory.
// Initialised on first boot to the default home location.
RTC_DATA_ATTR GpsFix lastFix = {
    {DEFAULT_LATITUDE, DEFAULT_LONGITUDE},
    0,       // timestamp
    0.0f,    // hdop
    0,       // satellites
    false    // valid
};

#ifdef DEBUG_LCD
I2C_LCD lcd(0x27, &Wire1);
#endif

// ============================================
// SETUP — runs on every wake from deep sleep
// ============================================

void setup() {
#ifdef DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println("Uncollar GPS Collar");
#endif

    Esp32PowerManager power;
    power.disableUnusedPeripherals();

    Wire1.begin(kI2c1Sda, kI2c1Scl);

    if (!configManager.begin()) {
#ifdef DEBUG_SERIAL
        Serial.println("Config init failed");
#endif
    }

#ifdef DEBUG_LCD
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Acquiring GPS...");
#endif

    AdafruitGpsManager gps(Wire1);
    gps.begin();
    gps.wake();
    delay(100);

#ifdef DEBUG_SERIAL
    Serial.println("Waiting for GPS fix...");
#endif

    bool hasFix = gps.waitForFix(GPS_FIX_TIMEOUT_MS);
    GpsFix fix  = gps.getLastFix();

    if (fix.valid) {
        lastFix = fix;
    }

    const GeoPoint& pos = fix.valid ? fix.position : lastFix.position;

    Polygon boundary(configManager.getBoundaryVertices(),
                     configManager.getBoundaryVertexCount());
    bool inside = boundary.contains(pos);

#ifdef DEBUG_SERIAL
    if (fix.valid) {
        Serial.print("Latitude:  "); Serial.println(pos.lat, 6);
        Serial.print("Longitude: "); Serial.println(pos.lon, 6);
        Serial.print("Satellites: "); Serial.println(fix.satellites);
    } else {
        Serial.println("No GPS fix — using last known position");
    }
    Serial.println(inside ? "Inside bounds" : "Outside bounds");
#endif

#ifdef DEBUG_LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(pos.lat, 4);
    lcd.setCursor(0, 1);
    lcd.print(inside ? "INSIDE" : "OUTSIDE");
#endif

    // LoRa: send position, listen for config update
    RFM95Radio radio(RFM95Config{kRadioCs, kRadioDio0, kRadioRst, 915.0f, 17});
    if (radio.begin()) {
        PositionReport report = {
            pos.lat, pos.lon,
            fix.valid ? fix.timestamp : lastFix.timestamp,
            fix.satellites,
            inside,
            0  // batteryMv: not yet measured
        };
        radio.sendPositionReport(report);

        ConfigUpdate update;
        if (radio.receiveConfigUpdate(update, CONFIG_RECEIVE_TIMEOUT_MS)) {
            configManager.setDefaultPosition(update.defaultLatitude,
                                             update.defaultLongitude);
            configManager.setBoundaryVertices(update.boundaryVertices,
                                              update.vertexCount);
            configManager.save();
        }
    }

    gps.sleep();
    power.enterDeepSleep(GPS_UPDATE_INTERVAL_US);
}

// ============================================
// LOOP — never reached due to deep sleep model
// ============================================

void loop() {
    Esp32PowerManager power;
    power.enterDeepSleep(GPS_UPDATE_INTERVAL_US);
}
