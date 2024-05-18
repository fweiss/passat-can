#include "app_mcp25625.h"
#include "indicator.h"

#include "esp_log.h"

static const char TAG[] = "app-mcp25625";

//forward
static void packLittleEndian(uint32_t identifier, uint8_t * const data);

AppMcp25625::AppMcp25625() : mcp25625() {}

AppMcp25625::~AppMcp25625() {}

void AppMcp25625::initBridge() {
    mcp25625.init();

    TickType_t const timerPeriod = pdMS_TO_TICKS(1000);
    const bool autoReload = false;
    heartbeatTimer = xTimerCreate("can heartbeat", timerPeriod, autoReload, nullptr, heartbeatFunction);
    canStatusTimer = xTimerCreate("can status", timerPeriod, true, (void*)this, canStatusFunction);
}
// deinitBridge

void AppMcp25625::startBridge() {
    ESP_LOGI(TAG, "start bridge");
    // run the following on APP CPU
    // the logging blocks the core and the ISR task can't finish before the next CAN frame is ready
    xTaskCreatePinnedToCore(webSocketSendTask, "can isr task", 4096, (void*)this, 1, NULL, APP_CPU_NUM);

    ESP_LOGI(TAG, "start can status timer %p", canStatusTimer);
    xTimerStart(canStatusTimer, 10);

    this->httpServer.onFrame = [this] (uint8_t * payload, size_t len) {
        ESP_LOGI(TAG, "websocket frame received");
        CanFrame frame{
            .identifier = 0x3d2,
            .length = 3,
            .data{0xaa, 0x55, 0xff},
            .extended = false,
            .remote = false,
        };   
        mcp25625.transmitFrame(frame);
    };
    
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
                static uint8_t data[payloadSize + 5];
                // data[0] = message.identifier & 0xff;
                // data[1] = (message.identifier >> 8) & 0xff;
                packLittleEndian(message.identifier, &data[0]);
                data[4] = (message.flags.srr << 0) | (message.flags.ide << 1);
                memcpy(&data[5], message.data, message.data_length_code);
                const int length = message.data_length_code + 5;
                self->httpServer.sendFrame(data, length);
            }
        }
    }
}

void AppMcp25625::heartbeatFunction(tmrTimerControl*) {
    Indicator::getInstance()->postState(Indicator::canbusNoHeartbeat);
}

void AppMcp25625::canStatusFunction(TimerHandle_t xTimer) {
    AppMcp25625 *self = static_cast<AppMcp25625*>(pvTimerGetTimerID(xTimer));
    if (self == nullptr) {
        ESP_LOGE(TAG, "canStatusFunction: self is null");
        return;
    }
    self->mcp25625.testReceiveStatus();

    const size_t payloadSize = 8;
    if (self->httpServer.isWebsocketConnected()) {
        static uint8_t data[payloadSize + 5];
        // data[0] = message.identifier & 0xff;
        // data[1] = (message.identifier >> 8) & 0xff;
        packLittleEndian(0x03fe, &data[0]);
        data[5] = 0xaa;
        // memcpy(&data[5], message.data, message.data_length_code);
        const int length = 1 + 5;
        self->httpServer.sendFrame(data, length);
    }
}

static void packLittleEndian(uint32_t identifier, uint8_t * const data) {
    data[0] = identifier & 0xff;
    data[1] = (identifier >> 8) & 0xff;
    data[2] = (identifier >> 16) & 0xff;
    data[3] = (identifier >> 24) & 0xff;
}
