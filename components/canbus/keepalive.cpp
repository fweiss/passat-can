#include "keepalive.hpp"

#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"

static const char TAG[] = "keepalive";

static CanFrame networkManagement {
    .identifier = 0x19b,
    .length = 8,
    .data{0,0,0xff,0xff,0xff,0xff,0xff,0xff},
    .extended = false,
    .remote = false,
};
static CanFrame centralLocking {
    .identifier = 0x3BE,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};
// aka KL15 status
// probably not sufficient to keep door module alive
static CanFrame gatewayAlive {
    .identifier = 0x3C0,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};
static CanFrame doorSTatusBroadcast {
    .identifier = 0x3C8,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};
CanFrame &bcmPresence = gatewayAlive;
static CanFrame combiCluster {
    .identifier = 0x3d0,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};
static CanFrame gatewayActive {
    .identifier = 0x575,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};
static CanFrame instumentClusterAlive {
    .identifier = 0x65D,
    .length = 8,
    .data{0,0,0,0,0,0,0,0},
    .extended = false,
    .remote = false,
};

void KeepAlive::simulatedGateway(MCP25625 *mcp25625) {
    // new KeepAlive(mcp25625, networkManagement, 10);
    // new KeepAlive(mcp25625, gatewayAlive, 10);
    // new KeepAlive(mcp25625, centralLocking, 20);
    // new KeepAlive(mcp25625, instumentClusterAlive, 100);
    // new KeepAlive(mcp25625, doorSTatusBroadcast, 10);

    new KeepAlive(mcp25625, bcmPresence, 20);
    // new KeepAlive(mcp25625, combiCluster, 20);
    // new KeepAlive(mcp25625, gatewayActive, 100);
}

KeepAlive::KeepAlive(MCP25625 *mcp25625) : mcp25625(mcp25625), canFrame(gatewayAlive), periodMillis(10)  {
    UBaseType_t priority = 1;
    xTaskCreatePinnedToCore(taskFunction, "keepalive task", 4096, (void*)this, priority, &taskHandle, APP_CPU_NUM);
}

KeepAlive::KeepAlive(MCP25625 *mcp25625, CanFrame canFrame, uint32_t periodMs) : mcp25625(mcp25625), canFrame(canFrame), periodMillis(periodMs)  {
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
        self->canFrame.data[0] = (self->canFrame.data[0] + 1) & 0x0f;
    }
}
