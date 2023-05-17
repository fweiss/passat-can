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
