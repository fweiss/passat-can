# Passat CAN
A little project to create a CAN Bus device that uses a web socket UI

## Wifi usage
runs in either station or access point mode.
GPIO pin 4 is used to select
- open = station
- grounded = access point

### Access point mode
Access point mode is available so connection can be made without connecting to an existing Wifi network.
The steps to connect in this mode are:
- insert jumper to pull pin 4 low
- power on or reset device
- go to internet/wifi settings on the client device
- select "wallop"
- open a browser at ``192.168.4.1``


The SSID is "wallop"

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

https://www.youtube.com/watch?v=J4XMFja1q0I

Extensive desription of WS
https://hpbn.co/websocket/

Pretty good explanation of the ESP32 WiFi connect process
https://esp32tutorials.com/esp32-esp-idf-connect-wifi-station-mode-example/

A WiFi Manager for ESP32 built with PIO
https://gitlab.com/prexus/wifi-manager

Good guide for accessing VAG CAN from the OBDII port
https://forum.macchina.cc/t/how-to-read-vw-can-bus/655

Basics of VW CAN bus system
http://www.volkspage.net/technik/ssp/ssp/SSP_238.pdf
