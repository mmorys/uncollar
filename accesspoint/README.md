# Uncollar — WiFi FTM Ranging Demo

A two-board proof-of-concept for measuring distance with **WiFi Fine Timing Measurement** (FTM, IEEE 802.11mc). One board acts as the WiFi access point + FTM *responder*; the other associates-free *initiator* measures the round-trip time of flight and converts it to a distance estimate.

This explores FTM as a possible short-range, GPS-independent way to tell how far the collar is from a base station (e.g. "is the dog in the yard?"). It is independent of the LoRa link used by `collar/` and `basestation/`.

## Hardware

- **2× Seeed Studio XIAO ESP32-C6** — the ESP32-C6 has the WiFi MAC/PHY support required for both the FTM initiator and responder roles.
- USB-C cables to the host (the boards expose a USB JTAG/serial port, enumerating as `/dev/ttyACM*`).

No wiring between the boards — ranging happens over the air. Place them anywhere from touching to tens of metres apart.

PlatformIO setup for the XIAO ESP32-C6 follows the [Seeed Studio guide](https://wiki.seeedstudio.com/xiao_esp32c6_with_platform_io/).

## Roles

| Environment | Source | Role | Default port |
|---|---|---|---|
| `ftm_responder` | `demos/ftm_responder.cpp` | Brings up a SoftAP (`FTM_Responder`, channel 1) with the 802.11mc FTM responder bit set. Idle otherwise — the WiFi driver answers FTM frames in the background. | `/dev/ttyACM0` |
| `ftm_initiator` | `demos/ftm_initiator.cpp` | Scans for the AP, runs an unassociated FTM burst against it, and prints the distance estimate from the driver's FTM report. | `/dev/ttyACM1` |

The two environments are flashed one to each board. If your boards enumerate in the opposite order, swap the `upload_port` / `monitor_port` values in [platformio.ini](platformio.ini) (or use `./assign_ports.sh` from the repo root to reassign by USB serial).

## Build & Flash

From the `accesspoint/` directory (or with `--project-dir accesspoint/` from the repo root):

```bash
# Build both
pio run

# Flash the responder to the first board, the initiator to the second
pio run -e ftm_responder --target upload
pio run -e ftm_initiator --target upload

# Watch the initiator's distance output (115200 baud)
pio device monitor -e ftm_initiator
```

## Expected Output

Responder:

```
FTM Responder — starting
AP up  SSID=FTM_Responder  channel=1  IP=192.168.4.1
FTM responder ready. Waiting for initiator...
[heartbeat] uptime=5s
```

Initiator (one line per FTM burst, every ~2 s):

```
Found AP  ch=1  BSSID=58:E6:C5:1A:F3:05  — initiating FTM
Distance: 0.15 m  (rtt_est=1 ps  dist_est=15 cm)
```

`dist_est` is the driver's distance estimate in centimetres; `rtt_est` is the estimated round-trip time in picoseconds.

## Tuning & Notes

- **`FTM_FRAMES_PER_BURST`** (initiator, default 16): more frames per burst means more averaging and lower noise, at the cost of a slower measurement. Valid values are driver-dependent (commonly 8/16/24/32/64).
- **`FTM_BURST_PERIOD`** (initiator, default 0 = let the driver choose): inter-burst spacing in units of 100 ms.
- **Accuracy**: FTM resolution is fundamentally limited by the 125 kHz–80 MHz channel bandwidth and multipath. At close range expect readings of 0–1 m that don't track small movements well; FTM is most useful over several metres in line-of-sight. Treat absolute values as approximate and calibrate per environment before relying on them.
- The initiator does **not** associate with the AP — FTM runs unassociated, so the responder needs no client connection.
- Both boards must be on the same WiFi channel; the initiator reads the channel from its scan result, so this is handled automatically.
