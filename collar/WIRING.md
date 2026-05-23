# Collar Wiring — QT Py ESP32-S3 ↔ RFM95W

## SPI Connection (RFM95W LoRa Radio)

The RFM95W connects over the ESP32-S3's FSPI hardware bus. All signals are 3.3 V logic.

| Signal       | ESP32-S3 GPIO | QT Py label | RFM95W pin | Notes                        |
|--------------|:-------------:|-------------|:----------:|------------------------------|
| Power        | —             | 3V3         | VCC        | 3.3 V only — do not use 5 V |
| Ground       | —             | GND         | GND        |                              |
| SPI clock    | 36            | SCK         | SCK        | FSPI bus                     |
| SPI data in  | 37            | MISO        | MISO       | FSPI bus                     |
| SPI data out | 35            | MOSI        | MOSI       | FSPI bus                     |
| Chip select  | 17            | A0          | NSS        | Active low, driven by RadioLib |
| Hardware RST | 18            | A1          | RST        | Active low                   |
| Packet IRQ   | 33            | A2          | DIO0       | Packet done / RX ready       |

> **Note:** DIO1 is not connected. The current firmware uses polling (`available()`)
> after `startReceive()`, so DIO1 is not required.

## Diagram

```mermaid
flowchart LR

subgraph esp["Adafruit QT Py ESP32-S3"]
V["3V3"]
G["GND"]
P0["SCK - GPIO 36"]
P1["MISO - GPIO 37"]
P2["MOSI - GPIO 35"]
P3["CS - GPIO 17 - A0"]
P4["RST - GPIO 18 - A1"]
P5["DIO0 - GPIO 33 - A2"]
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

## I2C Bus (existing — for reference)

GPS (PA1010D) and LCD share `Wire1` and do not conflict with SPI.

| Device      | Bus   | SDA GPIO | SCL GPIO | I2C Address |
|-------------|-------|:--------:|:--------:|:-----------:|
| GPS PA1010D | Wire1 | 41       | 40       | 0x10        |
| LCD (16×2)  | Wire1 | 41       | 40       | 0x27        |
