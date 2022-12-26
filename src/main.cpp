#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

#include "wifi.h"
#include "canbus.h"
#include "httpserver.h"
#include "mcp25625.h"

extern "C" {
	void app_main(void);
}

static const char *TAG = "passat-can";

void canReceiveTask(void * pvParameters);
void canReceiveMessageTask(void * args);
void wsTask(void * args);

WiFi wifi;
CanBus canbus;
HttpServer httpServer;
MCP25625 mcp25625;
QueueHandle_t receiveMessageQueue = xQueueCreate(10, sizeof(receive_msg_t));

void app_mainy() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    mcp25625.init();
  
    vTaskDelay(1000 / portTICK_PERIOD_MS);

}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs", // see partitons.csv
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

    ESP_ERROR_CHECK(esp_netif_init());
    wifi.startStation();
    // wifi.startAccessPoint();

    // size_t esp_netif_get_nr_of_ifs(void)
    ESP_LOGI(TAG, "number of interfaces %d", esp_netif_get_nr_of_ifs());

    HttpServer::onFrame = [] (uint8_t * payload, size_t len) {
        // translate WS message to CAN
        // canbus.triggerRead();
        twai_message_t msg;
        memset(&msg, 0, sizeof(msg));
        //littleendian
        msg.identifier = payload[0] + (payload[1] << 8);
        msg.data_length_code = 8;
        memcpy(&msg.data, &payload[2], TWAI_FRAME_MAX_DLC);
        canbus.sendFrame(msg);
    };
    httpServer.start();

#ifndef MCP25625_CAN
    canbus.onRecvFrame([] (twai_message_t & message) {
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
#endif

#ifdef MCP25625_CAN
    mcp25625.init();
    xTaskCreate(canReceiveTask, "can isr task", 4096, &mcp25625.receiveISRQueue, 1, NULL);
    // run the following on APP CPU
    // the logging blocks the core and the ISR task can't finish before the next CAN frame is ready
    xTaskCreatePinnedToCore(wsTask, "can isr task", 4096, &receiveMessageQueue, 1, NULL, APP_CPU_NUM);
    mcp25625.attachReceiveInterrupt();
    // mcp25625.testReceive();
    mcp25625.startReceiveMessages();
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // todo need a mutex
        mcp25625.testReceiveStatus();
    }
    mcp25625.deinit();
#endif


#ifndef MCP25625_CAN
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    while (true) {
        canbus.triggerRead();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
#endif

    // not yet implemented, placeholder
    // esp_netif_deinit();

    ESP_LOGI(TAG, "exit");
}

void canReceiveTask(void * pvParameters) {
    ESP_LOGI(TAG, "starting can receive task");

    QueueHandle_t *receiveISRQueue = static_cast<QueueHandle_t*>(pvParameters);

    while (true) {
        receive_msg_t zmessage;
        if (xQueueReceive(*receiveISRQueue, &zmessage, portMAX_DELAY)) {
            receive_msg_t message;
            bool success = mcp25625.receiveMessage(&message);
            if (success) {
                xQueueSend(receiveMessageQueue, &message, portMAX_DELAY);
            } else {
                ESP_LOGI(TAG, "nothing to receive");
            }
        }
    }
}

void canReceiveMessageTask(void * args) {
    receive_msg_t message;
    while (true) {
        if (xQueueReceive(receiveMessageQueue, &message, portMAX_DELAY)) {
            ESP_LOGI(TAG, "fid %x rtr %d dlc %d data %x %x %x %x %x %x %x %x", 
                message.identifier, message.rtr, message.data_length_code, 
                message.data[0], message.data[1], message.data[2], message.data[3],
                message.data[4], message.data[5], message.data[6], message.data[7]);
        }
    }
}

void wsTask(void * args) {
    receive_msg_t message;
    while (true) {
        if (xQueueReceive(receiveMessageQueue, &message, portMAX_DELAY)) {
            if (httpServer.isWebsocketConnected()) {
                static uint8_t data[TWAI_FRAME_MAX_DLC + 2];
                data[0] = message.identifier & 0xff;
                data[1] = (message.identifier >> 8) & 0xff;
                memcpy(&data[2], message.data, message.data_length_code);
                httpServer.sendFrame(data, message.data_length_code + 2);
            }
        }
    }
}

