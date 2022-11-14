# Passat CAN
A little project to create a CAN Bus device that uses a web socket UI

## Notes

project created with platform io

from esp idf github, examples/wifi/getting_started/station/main,
- copy to station_example_main.c to src/main.c
- copy Kconfig.projbuild to src

in main.c
comment out
``            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,

in platformio.ini, add
monitor_speed = 115200

``
build once, then run menuconfig
in Example Config, set WiFi SSID and WiFi Password

build and flash

run monitor

Then it should coonect and show the IP address

unfortunately, the log shows the wifi credentials

arch
start can bus controller
start http server
ws connect

wifi(tbd)
http_server(stopped, started, process_request, ws_connected, ws_receive_frame, ws_send_frame)
can_bus(stopped, started, receive_message, transmit_message)

## Links and references
A little Arduino-based project: https://www.youtube.com/watch?v=_Ajn560TLIo

A nice project similar to mine: https://github.com/MagnusThome/RejsaCAN-ESP32

https://docs.espressif.com/projects/esp-idf/en/release-v3.3/api-reference/peripherals/can.html

A simple demo of CAN to WS with ESP32 and MCP2515 for mining truck.
https://www.youtube.com/watch?v=05ihq09tkkQ

Nice PiHat from Copperhill
https://copperhilltech.com/raspberry-pi-4-4gb-with-pican-dual-can-bus-hat/

Pretty nice little CAN-Wifi project
https://github.com/NewTec-GmbH/esp32-can-iot

brief intro to j1939
https://copperhilltech.com/a-brief-introduction-to-the-sae-j1939-protocol

