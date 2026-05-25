# ConfigManager

Persistent configuration for the Uncollar collar firmware using ESP32 NVS (Non-Volatile Storage).

All values survive deep sleep and full power cycles. On first boot, defaults are written to NVS automatically.

## NVS Keys

Namespace: `uncollar_cfg`

| Key | Type | Default | Description |
|---|---|---|---|
| `cfg_lat` | `float` | `40.72272` | Home position latitude |
| `cfg_lon` | `float` | `-74.02116` | Home position longitude |
| `cfg_bnd_cnt` | `uint8` | `4` | Number of boundary polygon vertices |
| `cfg_bnd_N_lat` | `float` | *(square around default)* | Latitude of vertex N (0-based) |
| `cfg_bnd_N_lon` | `float` | *(square around default)* | Longitude of vertex N |
| `cfg_warn_aft` | `uint16` | `30` | Seconds outside boundary before first warning fires |
| `cfg_warn_rep` | `uint16` | `30` | Seconds between repeat warnings while still outside |
| `cfg_warn_act` | `uint8` | `0` | Warn actuator: `0` = beep, `1` = vibrate |
| `cfg_warn_on` | `bool` | `true` | Master enable for outside-boundary warnings |

## Interface

`ConfigManager` implements `IConfigManager`, which decouples collar logic from the NVS backend so the config layer can be mocked in native tests.

```cpp
// Initialization
bool begin();           // open NVS, load or write defaults
bool load();            // read from NVS (called by begin())
bool save();            // flush all fields to NVS
bool resetToDefaults(); // restore factory defaults and save

// Home position
GeoPoint getDefaultPosition() const;
void     setDefaultPosition(float lat, float lon);

// Geofence polygon
const GeoPoint* getBoundaryVertices()    const;
size_t          getBoundaryVertexCount() const;
bool            setBoundaryVertices(const GeoPoint* v, size_t count);

// Warning timing & action
uint16_t   getWarnAfterSeconds()  const;
uint16_t   getRepeatWarnSeconds() const;
WarnAction getWarnAction()        const;
bool       getWarningsEnabled()   const;

void setWarnAfterSeconds(uint16_t s);
void setRepeatWarnSeconds(uint16_t s);
void setWarnAction(WarnAction a);
void setWarningsEnabled(bool enabled);
```

## Constraints

- Boundary vertex count: 3–16 (`MIN_BOUNDARY_VERTICES` / `MAX_BOUNDARY_VERTICES`)
- `ConfigManager` allocates heap memory for the vertex array; it is freed on destruction or when `setBoundaryVertices` is called with a different count
- `WarnAction` is defined in `collar/lib/radio/radio.h` (shared with the wire protocol)
