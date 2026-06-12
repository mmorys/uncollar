# Uncollar — WiFi FTM Ranging Demo

A three-board proof-of-concept for measuring distance with **WiFi Fine Timing Measurement** (FTM, IEEE 802.11mc). Two boards act as WiFi access points + FTM *responders* (anchors); a third association-free *initiator* measures the round-trip time of flight to each anchor and converts it to a distance estimate.

This explores FTM as a possible short-range, GPS-independent way to tell how far the collar is from one or more fixed anchors (e.g. "is the dog in the yard?", or trilateration from several anchors). It is independent of the LoRa link used by `collar/` and `basestation/`.

## Hardware

- **2× Seeed Studio XIAO ESP32-C6** — the anchors. The ESP32-C6 provides the WiFi MAC/PHY support for the 802.11mc FTM responder role.
- **1× Adafruit QT Py ESP32-S3** — the mobile initiator. The ESP32-S3 supports the FTM initiator role and also drives the display.
- **1× I2C LCD1602** (SunFounder, address `0x27`) on the QT Py's STEMMA QT connector (`Wire1`, SDA 41 / SCL 40) — shows the live distance to each anchor, one per line.
- USB-C cables to the host (every board exposes a USB JTAG/serial port, enumerating as `/dev/ttyACM*`).

No wiring between the boards — ranging happens over the air. Place them anywhere from touching to tens of metres apart.

PlatformIO setup for the XIAO ESP32-C6 follows the [Seeed Studio guide](https://wiki.seeedstudio.com/xiao_esp32c6_with_platform_io/). The QT Py ESP32-S3 builds on the **same Seeed platform** using an ESP32-S3 board def — keeping the whole project on one platform avoids a package conflict with the espressif32 platform that `collar/` uses (see the note in [platformio.ini](platformio.ini)).

## Roles

| Environment | Source | Role | Default port |
|---|---|---|---|
| `ftm_responder_a` | `demos/ftm_responder.cpp` | XIAO ESP32-C6 anchor A. Brings up a SoftAP (`FTM_Anchor_A`, channel 1) with the 802.11mc FTM responder bit set. Idle otherwise — the WiFi driver answers FTM frames in the background. | `/dev/ttyACM1` |
| `ftm_responder_b` | `demos/ftm_responder.cpp` | XIAO ESP32-C6 anchor B. Same firmware as A, but the SSID (`FTM_Anchor_B`) and channel are injected via build flags. | `/dev/ttyACM2` |
| `ftm_initiator` | `demos/ftm_initiator.cpp` | QT Py ESP32-S3. Scans for every AP whose SSID starts with `FTM_Anchor_`, records each one's BSSID + channel, then loops ranging each anchor with an unassociated FTM burst. Shows the first two anchors' distances on the LCD (one per line, in metres) and mirrors everything to serial. | `/dev/ttyACM0` |

Both responders run the **same source file**; their identity differs only in the `ANCHOR_NAME` / `ANCHOR_CHANNEL` build flags set per environment in [platformio.ini](platformio.ini). Adding a fourth anchor is just another `ftm_responder_*` environment with a distinct SSID — the initiator discovers it automatically, no code change needed.

If your boards enumerate in a different order, swap the `upload_port` / `monitor_port` values in [platformio.ini](platformio.ini) (or use `./assign_ports.sh` from the repo root to reassign by USB serial).

## Build & Flash

From the `accesspoint/` directory (or with `--project-dir accesspoint/` from the repo root):

```bash
# Build everything
pio run

# Flash each role to its board
pio run -e ftm_responder_a --target upload
pio run -e ftm_responder_b --target upload
pio run -e ftm_initiator   --target upload

# Watch the initiator's distance output (115200 baud)
pio device monitor -e ftm_initiator
```

## Expected Output

Each responder (prints its own BSSID so you can match it in the initiator's discovery list):

```
FTM Responder — starting
AP up  SSID=FTM_Anchor_A  channel=1  BSSID=58:E6:C5:1A:F3:05  IP=192.168.4.1
FTM responder ready. Waiting for initiator...
[heartbeat] uptime=5s
```

Initiator (one block per measurement cycle, every ~2 s):

```
Discovered 2 anchor(s):
  [0] FTM_Anchor_A    ch=1  BSSID=58:E6:C5:1A:F3:05
  [1] FTM_Anchor_B    ch=1  BSSID=58:E6:C5:1A:DD:39
FTM_Anchor_A   : 0.45 m  (rtt_raw=3 ps  rtt_est=3 ps  dist_est=45 cm  frames=14)
FTM_Anchor_B   : 3.42 m  (rtt_raw=22 ps  rtt_est=22 ps  dist_est=342 cm  frames=14)
----------------------------------------
```

The LCD1602 mirrors the latest cycle, one anchor per line (the label is the SSID suffix, e.g. `FTM_Anchor_A` → `A`):

```
A: 0.45 m
B: 3.42 m
```

`dist_est` is the driver's distance estimate in centimetres; `rtt_raw`/`rtt_est` are the raw and filtered round-trip times in picoseconds; `frames` is the number of FTM frames actually exchanged in the burst (a useful health check — a non-zero `frames` with `dist_est=0` means the exchange succeeded but the peer is below the close-range floor, not that ranging failed).

## Tuning & Notes

- **Channel topology**: all anchors are on channel 1, so the initiator never has to hop channels between measurements — this is the simplest and fastest arrangement. The trade-off is mutual interference between anchors on the shared channel. To spread anchors across channels instead, change each anchor's `ANCHOR_CHANNEL` build flag; the initiator already reads each anchor's channel from the scan and passes it to `initiateFTM()`, so it will hop automatically — at the cost of extra latency per hop. Worth testing both ways.
- **Matching by BSSID**: the initiator keys each measurement off the anchor's BSSID (MAC), discovered once by scanning for the `FTM_Anchor_` SSID prefix. This is what lets it iterate over an arbitrary number of anchors uniformly.
- **`DEBUG_SERIAL`** (initiator): defined by default at the top of `ftm_initiator.cpp`. Comment it out to build a headless, LCD-only initiator — with it undefined the board never opens, waits on, or writes to the USB serial port, and all `DBG()`/`DBGLN()` calls compile to nothing. Handy once the LCD is the only readout you need.
- **`FTM_FRAMES_PER_BURST`** (initiator, default 16): more frames per burst means more averaging and lower noise, at the cost of a slower measurement. Valid values are driver-dependent (commonly 8/16/24/32/64).
- **`FTM_BURST_PERIOD`** (initiator, default 0 = let the driver choose): inter-burst spacing in units of 100 ms.
- **Accuracy**: FTM resolution is fundamentally limited by the channel bandwidth and multipath. At close range expect readings of 0–1 m that don't track small movements well; below roughly 25 cm the driver clamps `rtt_raw`/`dist_est` to **0** even though frames are still exchanged (verified on-bench — two anchors sitting on the same desk read `0.45 m` and `0.00 m` purely because of their relative spacing). FTM is most useful over several metres in line-of-sight, so separate the boards by a few metres to see meaningful, distinct distances. Treat absolute values as approximate and calibrate per environment before relying on them.
- The initiator does **not** associate with the APs — FTM runs unassociated, so the responders need no client connection.
