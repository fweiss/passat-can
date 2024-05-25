+# Hardware
Information about the hardware components

## Overview
The prototype consists of the following components:
- MCP25625 Click from MikroElektronika
- ESP32-S3-DevkitC-1 v1.0 from Esspressif
- buck convertor from Pololu
- OBDII-DB9 cable
- solderless prototyping board

## Olimex ESP32 DevKit Lipo

GPIO-21 = TX
GPIO-22 = RX

[pinout](https://www.olimex.com/Products/IoT/ESP32/ESP32-DevKit-LiPo/resources/ESP32-DevKit-Lipo-GPIOs.png)

## Power profile
ESP32 WROOM 32D module
3.3 V 500 mA

### Olimex ESP32 DevKit Lipo
USB
UNK
5 V? 500 mA?

Direct

### MCP 2551
VDD 7 V max
IDD 75 mA max

### MCP 25625
5V + 3.3V
XTAL + caps
filter caps

## 12 v buck 5 V
See digikey list

## Espressif dev board

### power
is it OK to power off the buck converter and the USB-C at the same time?
J1-VCC-5V
     |
UART +--|<-- VBUS (CP2102)
USB  +--|<-- VBUSB NC?
     +------ LDO  - VCC_3V3-ESP_3V3 (J1-ESP-3V3)

## Bosch OBD 1100
- STM32F091CCT6
- LM393
- NXP A1050 (high-speed CAN transceiver)
- 8L05A (linear voltage regulator)
- 25Q03213 (32 Mb seriasl NOR flash)

## Cable OBD-II to DB9
> The cable from Sparkfun was not wired correctly for the MCP-25625 Click.

### From ITCARDIAG store on Aliexpress
This has the correct wiring for the CANH and CANL signals compatible with the MCP25625 Click breakout board.
| MCP25625 Click | DB9F | OBD-II M | CAN |
|--- |--- |--- |--- |
| NC | 1 | 11 | 
| CANL | 2 | 14 | CAN L J2234
| GND | 3 | 5 | GND SIG
| NC | 4 | 8 | 
nc | 5 | 1 | 
GND | 6 | NC | J1850 BUS
CANH | 7 | 6 | CAN H J2234
NC | 8 | 3 | 
see note | 9 | 16 | BAT

Note: Modified Click board to bring this out to the pins.

### Sparkfun cable **DOES NOT WORK WITH MCP25625 Click**
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

## MIKROE MCP25625 Click
This is a handy breakout board that comes with MCP25625 controller/tansceiver
and a DB-9m connector.
There are adapter cables that interface the DB-9m connector to a ODBII connector.
- $24
- db9-m, SPI, UART?

I made two modifications so that the device can pick up 12V power from the ODBII connector:
- cut the trace to CLO
- wire 12V from the DB-9m to the CLO pin

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

[ESP32-S3-DEVKITC1 v1.0](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1-v1.0.html)

### ESP32-S3-DEVKITC1 v1.0
WROOM N8R2
SK68XXMINI-HS green is too bright cannot get yellow or orange.

## CAN hardware vendors
https://kvaser.com



