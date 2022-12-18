Olimex ESP32 DevKit Lipo

GPIO-21 = TX
GPIO-22 = RX

[pinout](https://www.olimex.com/Products/IoT/ESP32/ESP32-DevKit-LiPo/resources/ESP32-DevKit-Lipo-GPIOs.png)

## Power
ESP32 WROOM 32D module
3.3 V 500 mA

## Olimex ESP32 DevKit Lipo
USB
UNK
5 V? 500 mA?

Direct

## MCP 2551
VDD 7 V max
IDD 75 mA max

## MCP 25625
5V + 3.3V
XTAL + caps
filter caps

## 12 v buck 5 V
See digikey list

## MCP25625 click
$24

## Cable OBD-II to DB9

| Seed v2 | DB9F | OBD-II M | CAN |
|--- |--- |--- |--- |
| GND, J5 | 1 | 5 | GND SIG
| GND, J2 | 2 | 4 | GND CH
| CAB H, J3 | 3 | 6 | CAN H J2234
X | 4 | 7 | ISO 9141-2 K
CAN L J4 | 5 | 14 | CAN L J2234
X | 6 | 10 | J1850 BUS
PIN 7 | 7 | 2 | J1850 BUS +
X | 8 | 15 | ISO 9141-2 LOW
V-OBD9 | 9 | 16 | BAT

## MCP25625 Click
db9-m, SPI, UART

https://download.mikroe.com/documents/add-on-boards/click/mcp25625/mcp25625-click-schematic-v100.pdf

| pin | descrioption |
|--- |---
| CLO | PWM | 6 CLKOUT
| INT | INT | 25 INT
| TX0 | TX | 7 TX0RTS
| RX0 | RX | 24 RX0BF
| TX1 | SCL | 8 TX1RTS
| RX1 | SDA | 23 RX1BF
| 5V | 5V | 19 VDDA
| GND
| STB | AN | 15 STBY (via switch)
| RST | RST | 2 RST
| CS | CS | 1 CS
| SCK | SCK | 26 SCK
| SDO | MISO | 28 MISO
| SDI | MOSI | 27 MOSI
| 3V3
| GND

## Links and references
Great selection of breadboard breakout boards.
https://www.elabbay.com

## ESP-S3 links
https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html


