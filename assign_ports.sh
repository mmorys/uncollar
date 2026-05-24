#!/usr/bin/env bash
# Detects which /dev/ttyACM* port each board is on and updates both
# platformio.ini files accordingly.
#
# Collar:      Adafruit QT Py ESP32-S3  (VID 303a — Espressif JTAG/serial)
# Basestation: Adafruit QT Py ESP32 Pico (VID 1a86 — WCH CH34x UART bridge)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COLLAR_INI="$SCRIPT_DIR/collar/platformio.ini"
BASESTATION_INI="$SCRIPT_DIR/basestation/platformio.ini"

COLLAR_VID="303a"
BASESTATION_VID="1a86"

collar_port=""
basestation_port=""

for dev in /dev/ttyACM0 /dev/ttyACM1; do
    if [[ ! -e "$dev" ]]; then
        echo "WARNING: $dev not present, skipping" >&2
        continue
    fi
    vid=$(cat "/sys/class/tty/$(basename "$dev")/device/../idVendor" 2>/dev/null || true)
    case "${vid,,}" in
        "$COLLAR_VID")      collar_port="$dev" ;;
        "$BASESTATION_VID") basestation_port="$dev" ;;
        *) echo "WARNING: unrecognised VID '${vid}' on $dev, skipping" >&2 ;;
    esac
done

if [[ -z "$collar_port" ]]; then
    echo "ERROR: collar board (VID $COLLAR_VID) not found" >&2; exit 1
fi
if [[ -z "$basestation_port" ]]; then
    echo "ERROR: basestation board (VID $BASESTATION_VID) not found" >&2; exit 1
fi

echo "Collar:      $collar_port"
echo "Basestation: $basestation_port"

sed -i "s|upload_port = /dev/ttyACM[0-9]*|upload_port = $collar_port|g"  "$COLLAR_INI"
sed -i "s|monitor_port = /dev/ttyACM[0-9]*|monitor_port = $collar_port|g" "$COLLAR_INI"

sed -i "s|upload_port = /dev/ttyACM[0-9]*|upload_port = $basestation_port|g"  "$BASESTATION_INI"
sed -i "s|monitor_port = /dev/ttyACM[0-9]*|monitor_port = $basestation_port|g" "$BASESTATION_INI"

echo "Updated collar/platformio.ini and basestation/platformio.ini"
