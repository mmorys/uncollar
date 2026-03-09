/**
 * @file config_manager.h
 * @brief Configuration manager for Uncollar GPS collar using NVS storage.
 * 
 * Provides persistent storage for default location and geofence boundary
 * using ESP32's Non-Volatile Storage (NVS). On first boot, default values
 * are used and saved. Subsequent boots load from NVS.
 * 
 * @copyright Apache 2.0 License
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "../point_in_polygon/point_in_polygon.h"

// ============================================
// DEFAULT CONFIGURATION VALUES
// ============================================

// Default location (NYC - Central Park area as example)
constexpr float DEFAULT_LATITUDE = 40.72272f;
constexpr float DEFAULT_LONGITUDE = -74.02116f;

// Maximum number of boundary vertices supported
constexpr size_t MAX_BOUNDARY_VERTICES = 16;

// Minimum number of vertices for a valid polygon
constexpr size_t MIN_BOUNDARY_VERTICES = 3;

// Default geofence: ~100m x 100m square around default location
constexpr GeoPoint DEFAULT_BOUNDARY_VERTICES[] = {
    {DEFAULT_LATITUDE - 0.0005f, DEFAULT_LONGITUDE - 0.0005f},  // SW corner
    {DEFAULT_LATITUDE - 0.0005f, DEFAULT_LONGITUDE + 0.0005f},  // SE corner
    {DEFAULT_LATITUDE + 0.0005f, DEFAULT_LONGITUDE + 0.0005f},  // NE corner
    {DEFAULT_LATITUDE + 0.0005f, DEFAULT_LONGITUDE - 0.0005f}   // NW corner
};
constexpr size_t DEFAULT_BOUNDARY_VERTEX_COUNT = 
    sizeof(DEFAULT_BOUNDARY_VERTICES) / sizeof(DEFAULT_BOUNDARY_VERTICES[0]);

// NVS namespace and keys
constexpr char NVS_NAMESPACE[] = "uncollar_cfg";
constexpr char KEY_LATITUDE[] = "cfg_lat";
constexpr char KEY_LONGITUDE[] = "cfg_lon";
constexpr char KEY_BOUNDARY_COUNT[] = "cfg_bnd_cnt";
constexpr char KEY_BOUNDARY_PREFIX[] = "cfg_bnd_";

// ============================================
// CONFIG STRUCT
// ============================================

/**
 * @brief Configuration data structure.
 * 
 * Holds the default location and boundary vertices for geofencing.
 * The boundary vertices are dynamically allocated to allow runtime changes.
 */
struct Config {
    float defaultLatitude;
    float defaultLongitude;
    GeoPoint* boundaryVertices;
    size_t boundaryVertexCount;
};

// ============================================
// CONFIG MANAGER CLASS
// ============================================

/**
 * @brief Manages configuration persistence using ESP32 NVS.
 * 
 * This class provides an interface to load, save, and modify configuration
 * values that persist across power cycles. On first boot, default values
 * are used. When updated via LoRa (future feature), values persist.
 * 
 * @note This class allocates memory for boundary vertices. Ensure to call
 *       begin() before any other methods and handle memory appropriately.
 */
class ConfigManager {
public:
    /**
     * @brief Construct a new ConfigManager object.
     */
    ConfigManager();

    /**
     * @brief Destroy the ConfigManager object.
     * 
     * Frees allocated memory for boundary vertices.
     */
    ~ConfigManager();

    /**
     * @brief Initialize the config manager.
     * 
     * Must be called before any other methods. Loads configuration
     * from NVS or falls back to defaults if not found.
     * 
     * @return true if initialization successful, false on error
     */
    bool begin();

    /**
     * @brief Load configuration from NVS.
     * 
     * Reads values from NVS. If values are not found (first boot),
     * loads defaults and saves them to NVS.
     * 
     * @return true if loaded successfully, false on error
     */
    bool load();

    /**
     * @brief Save current configuration to NVS.
     * 
     * Persists all current configuration values to non-volatile storage.
     * 
     * @return true if saved successfully, false on error
     */
    bool save();

    /**
     * @brief Reset configuration to defaults.
     * 
     * Resets all values to default and saves to NVS.
     * 
     * @return true if reset successful, false on error
     */
    bool resetToDefaults();

    // ============================================
    // GETTERS
    // ============================================

    /**
     * @brief Get the default latitude.
     * @return Default latitude in decimal degrees.
     */
    float getDefaultLatitude() const;

    /**
     * @brief Get the default longitude.
     * @return Default longitude in decimal degrees.
     */
    float getDefaultLongitude() const;

    /**
     * @brief Get the boundary vertices.
     * @return Pointer to array of GeoPoint vertices.
     */
    const GeoPoint* getBoundaryVertices() const;

    /**
     * @brief Get the number of boundary vertices.
     * @return Number of vertices in the boundary polygon.
     */
    size_t getBoundaryVertexCount() const;

    /**
     * @brief Get the complete configuration struct.
     * @return Reference to the Config struct.
     */
    const Config& getConfig() const;

    // ============================================
    // SETTERS (for LoRa updates)
    // ============================================

    /**
     * @brief Set the default latitude.
     * @param lat Latitude in decimal degrees (-90 to 90).
     */
    void setDefaultLatitude(float lat);

    /**
     * @brief Set the default longitude.
     * @param lon Longitude in decimal degrees (-180 to 180).
     */
    void setDefaultLongitude(float lon);

    /**
     * @brief Set the boundary vertices.
     * 
     * Allocates new memory for vertices if count differs from current.
     * 
     * @param vertices Pointer to array of GeoPoint vertices.
     * @param count Number of vertices (must be >= MIN_BOUNDARY_VERTICES).
     * @return true if set successfully, false on invalid count.
     */
    bool setBoundaryVertices(const GeoPoint* vertices, size_t count);

    /**
     * @brief Check if configuration has been initialized.
     * @return true if begin() has been called successfully.
     */
    bool isInitialized() const;

private:
    Preferences _prefs;
    Config _config;
    bool _initialized;

    /**
     * @brief Load defaults into config.
     */
    void loadDefaults();

    /**
     * @brief Free allocated boundary memory.
     */
    void freeBoundaryMemory();
};

#endif // CONFIG_MANAGER_H
