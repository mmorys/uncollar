/**
 * @file config_manager.cpp
 * @brief Implementation of ConfigManager for NVS-based configuration storage.
 */

#include "config_manager.h"

// ============================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================

ConfigManager::ConfigManager()
    : _initialized(false) {
    // Initialize config with null values
    _config.defaultLatitude = 0.0f;
    _config.defaultLongitude = 0.0f;
    _config.boundaryVertices = nullptr;
    _config.boundaryVertexCount = 0;
}

ConfigManager::~ConfigManager() {
    freeBoundaryMemory();
}

// ============================================
// INITIALIZATION
// ============================================

bool ConfigManager::begin() {
    if (_initialized) {
        return true;
    }

    // Try to open NVS partition
    if (!_prefs.begin(NVS_NAMESPACE, false)) {
        #ifdef DEBUG_SERIAL
        Serial.println("Failed to open NVS namespace");
        #endif
        return false;
    }

    // Load configuration
    if (!load()) {
        #ifdef DEBUG_SERIAL
        Serial.println("Failed to load configuration");
        #endif
        return false;
    }

    _initialized = true;
    return true;
}

bool ConfigManager::isInitialized() const {
    return _initialized;
}

// ============================================
// LOAD / SAVE
// ============================================

void ConfigManager::loadDefaults() {
    freeBoundaryMemory();

    _config.defaultLatitude = DEFAULT_LATITUDE;
    _config.defaultLongitude = DEFAULT_LONGITUDE;
    _config.boundaryVertexCount = DEFAULT_BOUNDARY_VERTEX_COUNT;

    // Allocate and copy default boundary vertices
    _config.boundaryVertices = new GeoPoint[DEFAULT_BOUNDARY_VERTEX_COUNT];
    for (size_t i = 0; i < DEFAULT_BOUNDARY_VERTEX_COUNT; i++) {
        _config.boundaryVertices[i] = DEFAULT_BOUNDARY_VERTICES[i];
    }
}

bool ConfigManager::load() {
    // Check if configuration exists in NVS
    // We'll check if the latitude key exists
    if (!_prefs.isKey(KEY_LATITUDE)) {
        #ifdef DEBUG_SERIAL
        Serial.println("No config found in NVS, loading defaults");
        #endif
        
        // Load defaults
        loadDefaults();
        
        // Save defaults to NVS for next boot
        return save();
    }

    // Load from NVS
    _config.defaultLatitude = _prefs.getFloat(KEY_LATITUDE, DEFAULT_LATITUDE);
    _config.defaultLongitude = _prefs.getFloat(KEY_LONGITUDE, DEFAULT_LONGITUDE);
    _config.boundaryVertexCount = _prefs.getUChar(KEY_BOUNDARY_COUNT, DEFAULT_BOUNDARY_VERTEX_COUNT);

    // Validate count
    if (_config.boundaryVertexCount < MIN_BOUNDARY_VERTICES || 
        _config.boundaryVertexCount > MAX_BOUNDARY_VERTICES) {
        #ifdef DEBUG_SERIAL
        Serial.println("Invalid boundary count in NVS, loading defaults");
        #endif
        loadDefaults();
        return save();
    }

    // Allocate memory for boundary vertices
    freeBoundaryMemory();
    _config.boundaryVertices = new GeoPoint[_config.boundaryVertexCount];

    // Load each vertex
    char keyBuffer[32];
    for (size_t i = 0; i < _config.boundaryVertexCount; i++) {
        snprintf(keyBuffer, sizeof(keyBuffer), "%s%u_lat", KEY_BOUNDARY_PREFIX, i);
        _config.boundaryVertices[i].lat = _prefs.getFloat(keyBuffer, 0.0f);
        
        snprintf(keyBuffer, sizeof(keyBuffer), "%s%u_lon", KEY_BOUNDARY_PREFIX, i);
        _config.boundaryVertices[i].lon = _prefs.getFloat(keyBuffer, 0.0f);
    }

    #ifdef DEBUG_SERIAL
    Serial.println("Configuration loaded from NVS");
    Serial.print("Latitude: ");
    Serial.println(_config.defaultLatitude, 6);
    Serial.print("Longitude: ");
    Serial.println(_config.defaultLongitude, 6);
    Serial.print("Boundary vertices: ");
    Serial.println(_config.boundaryVertexCount);
    #endif

    return true;
}

bool ConfigManager::save() {
    // Save latitude and longitude
    _prefs.putFloat(KEY_LATITUDE, _config.defaultLatitude);
    _prefs.putFloat(KEY_LONGITUDE, _config.defaultLongitude);
    _prefs.putUChar(KEY_BOUNDARY_COUNT, _config.boundaryVertexCount);

    // Save each boundary vertex
    char keyBuffer[32];
    for (size_t i = 0; i < _config.boundaryVertexCount; i++) {
        snprintf(keyBuffer, sizeof(keyBuffer), "%s%u_lat", KEY_BOUNDARY_PREFIX, i);
        _prefs.putFloat(keyBuffer, _config.boundaryVertices[i].lat);
        
        snprintf(keyBuffer, sizeof(keyBuffer), "%s%u_lon", KEY_BOUNDARY_PREFIX, i);
        _prefs.putFloat(keyBuffer, _config.boundaryVertices[i].lon);
    }

    #ifdef DEBUG_SERIAL
    Serial.println("Configuration saved to NVS");
    #endif

    return true;
}

bool ConfigManager::resetToDefaults() {
    loadDefaults();
    return save();
}

// ============================================
// GETTERS
// ============================================

float ConfigManager::getDefaultLatitude() const {
    return _config.defaultLatitude;
}

float ConfigManager::getDefaultLongitude() const {
    return _config.defaultLongitude;
}

const GeoPoint* ConfigManager::getBoundaryVertices() const {
    return _config.boundaryVertices;
}

size_t ConfigManager::getBoundaryVertexCount() const {
    return _config.boundaryVertexCount;
}

const Config& ConfigManager::getConfig() const {
    return _config;
}

// ============================================
// SETTERS
// ============================================

void ConfigManager::setDefaultLatitude(float lat) {
    _config.defaultLatitude = lat;
}

void ConfigManager::setDefaultLongitude(float lon) {
    _config.defaultLongitude = lon;
}

bool ConfigManager::setBoundaryVertices(const GeoPoint* vertices, size_t count) {
    // Validate count
    if (count < MIN_BOUNDARY_VERTICES || count > MAX_BOUNDARY_VERTICES) {
        #ifdef DEBUG_SERIAL
        Serial.print("Invalid boundary vertex count: ");
        Serial.println(count);
        #endif
        return false;
    }

    // Validate pointer
    if (vertices == nullptr) {
        #ifdef DEBUG_SERIAL
        Serial.println("Null vertices pointer");
        #endif
        return false;
    }

    // Reallocate if count changed
    if (count != _config.boundaryVertexCount) {
        freeBoundaryMemory();
        _config.boundaryVertices = new GeoPoint[count];
    }

    // Copy vertices
    for (size_t i = 0; i < count; i++) {
        _config.boundaryVertices[i] = vertices[i];
    }

    _config.boundaryVertexCount = count;

    #ifdef DEBUG_SERIAL
    Serial.print("Boundary vertices updated: ");
    Serial.println(count);
    #endif

    return true;
}

// ============================================
// PRIVATE HELPERS
// ============================================

void ConfigManager::freeBoundaryMemory() {
    if (_config.boundaryVertices != nullptr) {
        delete[] _config.boundaryVertices;
        _config.boundaryVertices = nullptr;
    }
    _config.boundaryVertexCount = 0;
}
