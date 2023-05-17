# Firmware
Discussion of the firmware stack

## ESP32 CAN aka TWAI
built-in hardware controller
needs external transceiver
not compatible with ISO11898-1 FD Format frames, treats them as errors
use callback for receive
thin adapter layer

## MCP_CAN_lib
https://github.com/coryjfowler/MCP_CAN_lib
C++
uses polling receive
for Arduino, not esp-idf
uses arduino SPI.h

## MikroElektronika Click MCP25625
https://github.com/MikroElektronika/Click_MCP25625


## Links and references
https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf

interesting can-over-dc
https://yamar.com/datasheet/MD-DCAN500.pdf

shimano di2 shift over can bus

Microchip CAN products
https://microchipdeveloper.com/can:product-overview
