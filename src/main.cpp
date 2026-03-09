#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "esp_bt.h"
#include "I2C_LCD.h"
#include <Adafruit_GPS.h>
#include <esp_sleep.h>

// Uncomment to enable serial debugging output
#define DEBUG_SERIAL

// Uncomment to enable LCD debugging output
// #define DEBUG_LCD

// ============================================
// CONFIGURATION
// ============================================
constexpr uint32_t GPS_UPDATE_INTERVAL_SEC = 5;
constexpr uint32_t GPS_FIX_TIMEOUT_SEC = 3;
constexpr float DEFAULT_LATITUDE = 39.6416f;    // Default (configurable)
constexpr float DEFAULT_LONGITUDE = -84.0812f;

// Constructor: (I2C Address, Pointer to Wire interface)
I2C_LCD lcd(0x27, &Wire1);

// Connect to the GPS on the hardware I2C port
Adafruit_GPS GPS(&Wire1);

// ============================================
// RTC MEMORY - Persists across deep sleep
// ============================================
RTC_DATA_ATTR struct PositionData {
    float latitude;
    float longitude;
    uint32_t timestamp;
    bool hasValidFix;
} lastPosition = {
    DEFAULT_LATITUDE,
    DEFAULT_LONGITUDE,
    0,
    false
};

uint32_t timer = millis();

// ============================================
// POWER MANAGEMENT FUNCTIONS
// ============================================

/**
 * Disable unused ESP32 peripherals to save power
 * WiFi and Bluetooth are not needed - LoRa handles communication
 */
void disableUnusedPeripherals() {
    // Disable WiFi
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
    
    // Disable Bluetooth
    btStop();
    
    // Note: esp_bt_controller_disable() requires Bluetooth to be enabled in ESP-IDF
    // btStop() should be sufficient for most use cases
    // esp_bt_controller_disable();
    
    #ifdef DEBUG_SERIAL
    Serial.println("WiFi and Bluetooth disabled");
    #endif
}

/**
 * Put GPS into standby mode for power savings
 */
void gpsSleep() {
    GPS.sendCommand(PMTK_STANDBY);
    #ifdef DEBUG_SERIAL
    Serial.println("GPS put to sleep");
    #endif
}

/**
 * Wake GPS from standby mode
 */
void gpsWake() {
    GPS.sendCommand(PMTK_AWAKE);
    #ifdef DEBUG_SERIAL
    Serial.println("GPS woken up");
    #endif
}

/**
 * Store current GPS position to RTC memory for hot-start
 */
void storePosition(float lat, float lon) {
    lastPosition.latitude = lat;
    lastPosition.longitude = lon;
    lastPosition.timestamp = millis();
    lastPosition.hasValidFix = true;
    
    #ifdef DEBUG_SERIAL
    Serial.print("Position stored: ");
    Serial.print(lat, 6);
    Serial.print(", ");
    Serial.println(lon, 6);
    #endif
}

/**
 * Enter deep sleep for configured interval
 */
void enterDeepSleep() {
    #ifdef DEBUG_SERIAL
    Serial.print("Entering deep sleep for ");
    Serial.print(GPS_UPDATE_INTERVAL_SEC);
    Serial.println(" seconds...");
    #endif
    
    // Put GPS to sleep before ESP32 sleeps
    gpsSleep();
    
    // Configure timer wakeup
    esp_sleep_enable_timer_wakeup(GPS_UPDATE_INTERVAL_SEC * 1000000ULL);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

/**
 * Wait for GPS fix with timeout
 * Returns true if fix acquired, false if timeout
 */
bool waitForGpsFix(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    
    while (millis() - startTime < timeoutMs) {
        char c = GPS.read();
        if (GPS.newNMEAreceived()) {
            if (!GPS.parse(GPS.lastNMEA())) {
                continue;
            }
            if (GPS.fix) {
                return true;
            }
        }
    }
    
    return false;
}

// ============================================
// SETUP
// ============================================

void setup() {
    #ifdef DEBUG_SERIAL
    // Initialize serial port for debugging
    Serial.begin(115200);
    Serial.println("Uncollar GPS Collar - Power Optimized");
    #endif

    // Initialize the I2C bus
    Wire1.begin(41, 40);

    // Disable unused peripherals first
    disableUnusedPeripherals();

    #ifdef DEBUG_LCD
    // Initialize the LCD
    lcd.begin(16, 2);
    lcd.backlight();
    #endif
    
    #ifdef DEBUG_SERIAL
    Serial.println("Initializing GPS...");
    #endif

    // Initialize the GPS
    GPS.begin(0x10);  // The I2C address to use is 0x10
    
    // Use RMC only for efficient latitude/longitude data
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    
    // Set the update rate to 1 Hz
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    // Wake GPS from any previous sleep
    gpsWake();
    
    delay(100); // Give GPS time to respond

    #ifdef DEBUG_LCD
    lcd.setCursor(0, 0);
    lcd.print("Acquiring GPS...");
    #endif
   
    #ifdef DEBUG_SERIAL
    Serial.println("Waiting for GPS fix...");
    #endif

    // Wait for GPS fix with timeout
    bool gotFix = waitForGpsFix(GPS_FIX_TIMEOUT_SEC * 1000);
    
    if (gotFix && GPS.fix) {
        // Convert latitude from DDMM.MMMMM to decimal degrees
        int lat_degrees = (int)(GPS.latitude / 100);
        float lat_minutes = GPS.latitude - (lat_degrees * 100);
        float lat_decimal = lat_degrees + (lat_minutes / 60.0);
        if (GPS.lat == 'S') lat_decimal = -lat_decimal;
        
        // Convert longitude from DDMM.MMMMM to decimal degrees
        int lon_degrees = (int)(GPS.longitude / 100);
        float lon_minutes = GPS.longitude - (lon_degrees * 100);
        float lon_decimal = lon_degrees + (lon_minutes / 60.0);
        if (GPS.lon == 'W') lon_decimal = -lon_decimal;
        
        // Store position for next hot-start
        storePosition(lat_decimal, lon_decimal);
        
        #ifdef DEBUG_LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(lat_decimal, 4);
        lcd.setCursor(0, 1);
        lcd.print(lon_decimal, 4);
        #endif
        
        #ifdef DEBUG_SERIAL
        Serial.print("Latitude: ");
        Serial.print(lat_decimal, 6);
        Serial.println(GPS.lat);
        Serial.print("Longitude: ");
        Serial.print(lon_decimal, 6);
        Serial.println(GPS.lon);
        Serial.println("Fix acquired!");
        #endif
    } else {
        #ifdef DEBUG_LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No fix - using");
        lcd.setCursor(0, 1);
        lcd.print("last position");
        #endif
        
        #ifdef DEBUG_SERIAL
        Serial.println("No GPS fix - using last known position");
        Serial.print("Last position: ");
        Serial.print(lastPosition.latitude, 6);
        Serial.print(", ");
        Serial.println(lastPosition.longitude, 6);
        #endif
    }

    // Enter deep sleep cycle
    // On wake, we'll have a cold start for GPS but position helps
    enterDeepSleep();
}

// ============================================
// MAIN LOOP - Never reached due to deep sleep
// ============================================

void loop() {
    // This should never be executed
    // ESP32 wakes from deep sleep and runs setup() again
    #ifdef DEBUG_SERIAL
    Serial.println("ERROR: Loop executed - this should not happen!");
    #endif
    
    // Emergency: re-enter deep sleep
    enterDeepSleep();
}
