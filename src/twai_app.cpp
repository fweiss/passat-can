#include "twai_app.h"

#include "esp_log.h"

static char const * const TAG = "twai-app";

TwaiApp::TwaiApp() : 
    canbus() { }

void TwaiApp::initBridge() {
    canbus.onRecvFrame([&] (twai_message_t & message) {
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
    ESP_LOGI(TAG, "init canbus from initBridge");
    canbus.init();
}

void TwaiApp::start() {
    App::start();
    
    while (true) {
    canbus.triggerRead();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
