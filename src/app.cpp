#include "app.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_netif.h"

static const char *TAG = "app";

App::App() : httpServer() {}

App::~App() {}

void App::init() {
    esp_err_t err;

    initNvs(); // may need to store wifi credentials
    initSpiffs(); // used for web server

    err = esp_netif_init();
    ESP_ERROR_CHECK(err);

    wifi.startStation();
    // wifi.startAccessPoint();

    // initHttpServer();

    ESP_LOGI(TAG, "init bridge");
    // initBridge();
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

void App::start() {
    httpServer.start();
}
