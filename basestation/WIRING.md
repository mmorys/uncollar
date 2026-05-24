# Basestation Wiring — QT Py ESP32 Pico ↔ RFM95W

## SPI Connection (RFM95W LoRa Radio)

The RFM95W connects over the ESP32's HSPI (SPI2) hardware bus. All signals are 3.3 V logic.

| Signal       | ESP32 GPIO | QT Py label | RFM95W pin | Notes                          |
|--------------|:----------:|-------------|:----------:|--------------------------------|
| Power        | —          | 3V3         | VCC        | 3.3 V only — do not use 5 V   |
| Ground       | —          | GND         | GND        |                                |
| SPI clock    | 14         | SCK         | SCK        | HSPI bus                       |
| SPI data in  | 12         | MISO        | MISO       | HSPI bus                       |
| SPI data out | 13         | MOSI        | MOSI       | HSPI bus                       |
| Chip select  | 26         | A0          | NSS        | Active low, driven by ESPHome  |
| Hardware RST | 25         | A1          | RST        | Active low                     |
| Packet IRQ   | 27         | A2          | DIO0       | Packet done / RX ready         |

> **Note:** DIO1 is not connected. ESPHome's sx127x component uses interrupt-driven DIO0 for receive notification; DIO1 is not required.

## Diagram

```mermaid
flowchart LR

subgraph esp["Adafruit QT Py ESP32 Pico"]
V["3V3"]
G["GND"]
P0["SCK  - GPIO 14"]
P1["MISO - GPIO 12"]
P2["MOSI - GPIO 13"]
P3["CS   - GPIO 26 - A0"]
P4["RST  - GPIO 25 - A1"]
P5["DIO0 - GPIO 27 - A2"]
end

subgraph rfm["RFM95W Module"]
R0["VCC"]
R1["GND"]
R2["SCK"]
R3["MISO"]
R4["MOSI"]
R5["NSS"]
R6["RST"]
R7["DIO0"]
end

V -->|3.3V power| R0
G -->|Ground| R1
P0 -->|SPI clock| R2
P1 -->|SPI data in| R3
P2 -->|SPI data out| R4
P3 -->|Chip select| R5
P4 -->|Hardware reset| R6
P5 -->|Packet IRQ| R7
```
