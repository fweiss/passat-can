#include "keepalive.hpp"

#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"

static const char TAG[] = "keepalive";

// aka KL15 status
static CanFrame gatewayAlive {
    .identifier = 0x3C0,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};

KeepAlive::KeepAlive(MCP25625 *mcp25625) : mcp25625(mcp25625), canFrame(gatewayAlive), periodMillis(10)  {
    UBaseType_t priority = 1;
    xTaskCreatePinnedToCore(taskFunction, "keepalive task", 4096, (void*)this, priority, &taskHandle, APP_CPU_NUM);
}

KeepAlive::~KeepAlive() {
    stop();
    vTaskDelete(taskHandle);
}

void KeepAlive::start() {
    vTaskResume(taskHandle);
}

void KeepAlive::stop() {
    vTaskSuspend(taskHandle);
}

void KeepAlive::taskFunction(void* pvParameters) {
    if (pvParameters == nullptr) {
        ESP_DRAM_LOGE(TAG, "fuzzer: pvParameters is null");
        return;
    }
    KeepAlive* self = (KeepAlive*)pvParameters;
    while (true) {
        vTaskDelay(self->periodMillis / portTICK_PERIOD_MS);
        self->mcp25625->transmitFrame(self->canFrame);
    }
}
