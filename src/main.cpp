/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <esp_http_server.h>
#include "esp_spiffs.h"

#include "wifi.h"
#include "canbus.h"
#include "httpserver.h"

extern "C" {
	void app_main(void);
}
static const char *TAG = "passat-can";

WiFi wifi;
CanBus canbus;
HttpServer httpServer;

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(esp_netif_init());
    wifi.startStation();

    // size_t esp_netif_get_nr_of_ifs(void)
    ESP_LOGI(TAG, "number of interfaces %d", esp_netif_get_nr_of_ifs());

    HttpServer::onFrame = [] (uint8_t * payload, size_t len) {
        // translate WS message to CAN
        // canbus.triggerRead();
        can_message_t msg;
        memset(&msg, 0, sizeof(msg));
        //littleendian
        msg.identifier = payload[0] + (payload[1] << 8);
        msg.data_length_code = 8;
        memcpy(&msg.data, &payload[2], TWAI_FRAME_MAX_DLC);
        canbus.sendFrame(msg);
    };
    httpServer.start();

    canbus.onRecvFrame([] (can_message_t & message) {
        //Process received message
        // if (message.flags & CAN_MSG_FLAG_EXTD) {
        //     ESP_LOGI(TAG, "Message is in Extended Format");
        // } else {
        //     ESP_LOGI(TAG, "Message is in Standard Format");
        // }
        // ESP_LOGI(TAG, "%s", canbus.messageToString(message).c_str());

        if (httpServer.isWebsocketConnected()) {
            // httpServer.sendFrame(canbus.messageToString(message).c_str());
            static uint8_t data[TWAI_FRAME_MAX_DLC + 2];
            // big/little endian?
            data[0] = message.identifier & 0xff;
            data[1] = (message.identifier >> 8) & 0xff;
            memcpy(&data[2], message.data, message.data_length_code);
            httpServer.sendFrame(data, message.data_length_code + 2);
        }
    });
    canbus.init();

    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    while (true) {
        canbus.triggerRead();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // not yet implemented, placeholder
    // esp_netif_deinit();

    ESP_LOGI(TAG, "exit");
}
