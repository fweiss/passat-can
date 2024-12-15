#include "app_mcp25625.h"
#include "indicator.h"
#include "mcp25625.h"

#include "esp_log.h"

static const char TAG[] = "app-mcp25625";

//forward
static void packLittleEndian(uint32_t identifier, uint8_t * const data);

AppMcp25625::AppMcp25625() : mcp25625() {
    esp_log_level_set(TAG, ESP_LOG_INFO);
}

AppMcp25625::~AppMcp25625() {}

void AppMcp25625::initBridge() {
    mcp25625.init();

    TickType_t const timerPeriod = pdMS_TO_TICKS(1000);
    const bool autoReload = false;
    heartbeatTimer = xTimerCreate("can heartbeat", timerPeriod, autoReload, nullptr, heartbeatFunction);
    canStatusTimer = xTimerCreate("can status", timerPeriod, true, (void*)this, canStatusFunction);
    fuzzingTimer = xTimerCreate("can fuzzing", pdMS_TO_TICKS(100), true, (void*)this, fuzzingFunction);

    // todo check error
    // xTaskCreatePinnedToCore(canReceiveFrameTaskFunction, "can receive task", 2048, (void*)this, 1, &canReceiveFrameTask, APP_CPU_NUM);
    xTaskCreatePinnedToCore(canTransmitFrameTaskFunction, "can transmit task", 4096, (void*)this, 1, &canTransmitFrameTask, APP_CPU_NUM);
    xTaskCreatePinnedToCore(canErrorTaskFunction, "can error task", 4096, (void*)this, 1, &canErrorTask, APP_CPU_NUM);
}
// deinitBridge

void AppMcp25625::startBridge() {
    ESP_LOGI(TAG, "start bridge");
    // run the following on APP CPU
    // the logging blocks the core and the ISR task can't finish before the next CAN frame is ready
    xTaskCreatePinnedToCore(webSocketSendTask, "can isr task", 4096, (void*)this, 1, NULL, APP_CPU_NUM);

    // ESP_LOGI(TAG, "start can status timer %p", canStatusTimer);
    xTimerStart(canStatusTimer, 10);
    // xTimerStart(fuzzingTimer, 100);

    this->httpServer.onFrame = [this] (uint8_t * payload, size_t len) {
        ESP_LOGI(TAG, "websocket frame received");
        CanFrame frame{
            .identifier = 0x3d1,
            .length = 8,
            .data{},
            .extended = false,
            .remote = true,
        };   
        // mcp25625.transmitFrame(frame);
        xTimerStart(this->fuzzingTimer, 100);
    };
    
    mcp25625.attachInterrupt();
    // mcp25625.testReceive();
    mcp25625.startReceiveMessages();
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // todo need a mutex
        // mcp25625.testReceiveStatus();
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

struct WSFrame {
    uint8_t opcode;
    uint8_t payloadLength;
    uint8_t payload[125];
};

// @depracate - for testing only
void AppMcp25625::canStatusFunction(TimerHandle_t xTimer) {
    AppMcp25625 *self = static_cast<AppMcp25625*>(pvTimerGetTimerID(xTimer));
    if (self == nullptr) {
        ESP_LOGE(TAG, "canStatusFunction: self is null");
        return;
    }
    // probably cannot log in timer callback
    // self->mcp25625.testReceiveStatus();

    const size_t payloadSize = 8;
    if (self->httpServer.isWebsocketConnected()) {
        static uint8_t data[payloadSize + 5];
        // data[0] = message.identifier & 0xff;
        // data[1] = (message.identifier >> 8) & 0xff;
        packLittleEndian(0x03fe, &data[0]);

        CanStatus status;
        self->mcp25625.getStatus(status);

        memcpy(&data[5], &status, 5);
        const int length = 5 + 5;
        self->httpServer.sendFrame(data, length);
    }
}

void AppMcp25625::fuzzingFunction(TimerHandle_t xTimer) {
    AppMcp25625 *self = static_cast<AppMcp25625*>(pvTimerGetTimerID(xTimer));
    if (self == nullptr) {
        ESP_DRAM_LOGE(TAG, "fuzzingFunction: self is null");
        return;
    }
    // 0x181 window 
    static CanFrame canFrame{
        .identifier = 0x181,
        .length = 3,
        // .data{0x20,0x11,0x0},
        .data{0x80,0x44,0x0},
        .extended = false,
        .remote = false,
    };
    ESP_DRAM_LOGI(TAG, "fuzzingFunction %lx", canFrame.identifier);
    self->mcp25625.transmitFrame(canFrame);
    canFrame.identifier += 1;

    // uint8_t ctrl;
    // self->mcp25625.readRegister(MCP25625::reg::TXB0CTRL, ctrl);
    // uint8_t eflg;
    // self->mcp25625.readRegister(MCP25625::reg::EFLG, eflg);
    // ESP_DRAM_LOGI(TAG, "TXB0CTRL %x", eflg);
}

void AppMcp25625::canReceiveFrameTaskFunction(void * pvParameters) {
    AppMcp25625 * const self = static_cast<AppMcp25625*>(pvParameters);
    while (true) {
        receive_msg_t message;
        if (xQueueReceive(self->mcp25625.receiveMessageQueue, &message, portMAX_DELAY)) {
            // webSocketReceiveFrame
            // webSocketFrameQueue
        }
    }
}

void AppMcp25625::canTransmitFrameTaskFunction(void * pvParameters) {
    AppMcp25625 * const self = static_cast<AppMcp25625*>(pvParameters);
    while (true) {
        CanStatus canStatus;
        if (xQueueReceive(self->mcp25625.transmitFrameQueue, &canStatus, portMAX_DELAY)) {
            // ESP_LOGE(TAG, "transmit status canintf: %x caninte: %x eflg %x tec %d icod %x", 
            //     canStatus.canintf, canStatus.caninte, canStatus.eflg, canStatus.tec, canStatus.icod);

            // webSocketTransmitFrame
            // webSocketFrameQueue
        }
    }
}

void AppMcp25625::canErrorTaskFunction(void * pvParameters) {
    AppMcp25625 * const self = static_cast<AppMcp25625*>(pvParameters);
    while (true) {
        CanStatus canStatus;
        if (xQueueReceive(self->mcp25625.errorQueue, &canStatus, portMAX_DELAY)) {
            ESP_LOGE(TAG, "error status canintf: %x caninte: %x eflg %x tec %d tboctrl %x icod %x", 
                canStatus.canintf, canStatus.caninte, canStatus.eflg, canStatus.tec, canStatus.tb0ctrl, canStatus.icod);

            // webSocketErrorFrame
            // webSocketFrameQueue
        }
    }
}

static void packLittleEndian(uint32_t identifier, uint8_t * const data) {
    data[0] = identifier & 0xff;
    data[1] = (identifier >> 8) & 0xff;
    data[2] = (identifier >> 16) & 0xff;
    data[3] = (identifier >> 24) & 0xff;
}
