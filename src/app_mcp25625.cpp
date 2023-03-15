#include "app_mcp25625.h"
#include "indicator.h"

#include "esp_log.h"

static const char TAG[] = "app-mcp25625";

AppMcp25625::AppMcp25625() : mcp25625() {}

AppMcp25625::~AppMcp25625() {}

void AppMcp25625::initBridge() {
    mcp25625.init();

    TickType_t const timerPeriod = pdMS_TO_TICKS(1000);
    const bool autoReload = false;
    heartbeatTimer = xTimerCreate("can heartbeat", timerPeriod, autoReload, nullptr, heartbeatFunction);
}
// deinitBridge

void AppMcp25625::startBridge() {
    ESP_LOGI(TAG, "start bridge");
    // run the following on APP CPU
    // the logging blocks the core and the ISR task can't finish before the next CAN frame is ready
    xTaskCreatePinnedToCore(webSocketSendTask, "can isr task", 4096, (void*)this, 1, NULL, APP_CPU_NUM);
    
    mcp25625.attachReceiveInterrupt();
    // mcp25625.testReceive();
    mcp25625.startReceiveMessages();
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // todo need a mutex
        mcp25625.testReceiveStatus();
    }
    ESP_LOGI(TAG, "bridge shutting down");
    mcp25625.deinit();
}
// stopBridge

void AppMcp25625::webSocketSendTask(void * pvParameters) {
    AppMcp25625 * const self = static_cast<AppMcp25625*>(pvParameters);

    ESP_LOGI(TAG, "start websocket send task");
    const int payloadSize = 8;

    receive_msg_t message;
    while (true) {
        if (xQueueReceive(self->mcp25625.receiveMessageQueue, &message, portMAX_DELAY)) {
            // fixme, timeout? check error
            xTimerReset(self->heartbeatTimer, 10);
            Indicator::getInstance()->postState(Indicator::canbusHeartbeat);

            if (self->httpServer.isWebsocketConnected()) {
                static uint8_t data[payloadSize + 2];
                data[0] = message.identifier & 0xff;
                data[1] = (message.identifier >> 8) & 0xff;
                memcpy(&data[2], message.data, message.data_length_code);
                self->httpServer.sendFrame(data, message.data_length_code + 2);
            }
        }
    }
}

void AppMcp25625::heartbeatFunction(tmrTimerControl*) {
    Indicator::getInstance()->postState(Indicator::canbusNoHeartbeat);
}
