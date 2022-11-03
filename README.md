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

## Links and references
A little Arduino-based project: https://www.youtube.com/watch?v=_Ajn560TLIo
