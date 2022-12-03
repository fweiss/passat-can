#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

// redefine these in local.h instead of via menuconfig
// menuconfig unfortunately saves these secrets in sdkconfig
#undef CONFIG_ESP_WIFI_SSID
#undef CONFIG_ESP_WIFI_PASSWORD
#include "local.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

class WiFi {
public:
    WiFi();
    virtual ~WiFi();

    void startStation();
private:
    EventGroupHandle_t s_wifi_event_group;
    int s_retry_num;
    // const int EXAMPLE_ESP_MAXIMUM_RETRY;

    const EventBits_t WIFI_CONNECTED_BIT = BIT0; // we are connected to the AP with an IP
    const EventBits_t WIFI_FAIL_BIT      = BIT1; // we failed to connect after the maximum amount of retries

    void registerAnyId();
    void registerGotIp();
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void waitForConnection();
};
