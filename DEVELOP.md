# Develop
Notes for developer on configuring, building and modifying the firmware

## PlatformIO
This firmware is developed using PlatformIO, but another toolchain based on esp-idf could be used.

## Configure WiFi for station mode
The WiFi SSID and WPA password are needed to connect.
This information is not stored in the repository for security reasons.

Create a gitignored file ``src/local.h``, then follow the instructions in ``include/wifi.h`` to configure WiFi.

## Links and references
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/file_serving/main/file_server.c

[non-blocking TWAI read](https://github.com/atanisoft/OpenMRNIDF/blob/main/src/freertos_drivers/esp32/Esp32HardwareTwai.cpp)
