#include "app.h"
#include "indicator.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "driver/gpio.h"

static const char *TAG = "app";

App::App() : httpServer() {}

App::~App() {}

void App::init() {
    esp_err_t err;

    Indicator::getInstance()->setColor(100, 0, 0);

    initNvs(); // may need to store wifi credentials
    initSpiffs(); // used for web server

    initWifiModeSwitch();

    err = esp_netif_init();
    ESP_ERROR_CHECK(err);

    switch (wifiMode) {
        case STATION:
            wifi.startStation();
            break;
        case ACCESS_POINT:
            wifi.startAccessPoint();
            vTaskDelay(5000 / portTICK_PERIOD_MS); // give remote device time to connect
            break;
    }

    // initHttpServer();

    ESP_LOGI(TAG, "init bridge");
    initBridge();
}

void App::initNvs() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void App::initSpiffs() {
    esp_err_t err;

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs", // see partitons.csv
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    err = esp_vfs_spiffs_register(&conf);

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(err));
        }
        return;
    }
}

void App::initBridge() {
    // this should be overridden to use either
    // initTwai();
    // initMcp25625();
    // and wire up the WS events with the CAN bus events
    // may also need some synchronization with
    // wifi bringup
    // WS connection
}

void App::startBridge() {
    
}

void App::start() {
    httpServer.start();
    startBridge();
}

// connect the pin to ground for access point mode
void App::initWifiModeSwitch() {
    const gpio_num_t wifiModePin = GPIO_NUM_4;
    esp_rom_gpio_pad_select_gpio(wifiModePin);
    gpio_set_direction(wifiModePin, GPIO_MODE_INPUT);
    gpio_pulldown_dis(wifiModePin);
    gpio_pullup_en(wifiModePin);

    int level = gpio_get_level(wifiModePin);
    wifiMode = (level == 0 ? ACCESS_POINT : STATION);
}
