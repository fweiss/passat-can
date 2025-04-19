#include "fuzzer.hpp"
#include "esp_log.h"

static const char TAG[] = "Fuzzer";

Fuzzer::Fuzzer(MCP25625 *mcp25625) : periodMillis(2000), mcp25625(mcp25625) {
    UBaseType_t priority = 1;
    xTaskCreatePinnedToCore(taskFunction, "can fuzzing task", 4096, (void*)this, priority, &taskHandle, APP_CPU_NUM);
}

Fuzzer::~Fuzzer() {
    stop();
    vTaskDelete(taskHandle);
}

void Fuzzer::start() {
    vTaskResume(taskHandle);
}

void Fuzzer::stop() {
    vTaskSuspend(taskHandle);
}

void Fuzzer::taskFunction(void* pvParameters) {
    if (pvParameters == nullptr) {
        ESP_DRAM_LOGE(TAG, "fuzzer: pvParameters is null");
        return;
    }
    Fuzzer* fuzzer = (Fuzzer*)pvParameters;
    while (true) {
        vTaskDelay(fuzzer->periodMillis / portTICK_PERIOD_MS);
        fuzzer->fuzzingFunction();
    }
}

static CanFrame canFrameRTR{
    .identifier = 0xdd, //0x181,
    .length = 0,
    .data{},
    .extended = false,
    .remote = true,
};

void Fuzzer::fuzzingFunction() {
    // 0x181 window 
    static CanFrame canFrame{
        // SDS1104X-E trigger range 0-255
        .identifier = 0xdd, //0x181,
        .length = 3,
        // .data{0x20,0x11,0x0},
        .data{0x80,0x44,0x0},
        .extended = false,
        .remote = false,
    };
    CanFrame &transmitFrame = canFrame;
    // ESP_DRAM_LOGI(TAG, "fuzzingFunction %lx", canFrame.identifier);
    // transmitFrame.identifier = runningIdentifier++;
    mcp25625->transmitFrame(transmitFrame);

    // todo move to mcp25625
    const uint8_t TXREQ = 0x08;
    const uint8_t ABRT= 0x40;
    const uint8_t TXERR = 0x10;
    const uint8_t MLOA = 0x20;

    // wait until TXREQ == 0
    // todo also ABRT, TXERR, MLOA
    uint8_t ctrl;
    while(true) {
        mcp25625->readRegister(MCP25625::reg::TXB0CTRL, ctrl);
        if ((ctrl & 0x08) == 0) {
            break;
        }
    }
    uint8_t tec;
    mcp25625->readRegister(MCP25625::reg::TEC, tec);

    // const uint8_t TXERR = 0x10;
    if (ctrl & 0xff || true) {
        ESP_DRAM_LOGI(TAG, "id: 0x%x TXB0CTRL: 0x%x TEC: %d", transmitFrame.identifier, ctrl, tec);
    }

    // // wait send TXREQ == 0 (TX0, TX1, TX2)
    // // read staus doesn't seem to work
    // while (true) {
    //     uint8_t status;
    //     mcp25625->readStatusRegister(status);
    //     if ((status & (0x40 | 0x10 | 0x04)) == 0x0) {
    //         break;
    //     }
    // }
    // // could just read TXREQ here too
    // uint8_t ctrl;
    // mcp25625->readRegister(MCP25625::reg::TXB0CTRL, ctrl);
    // const uint8_t TXERR = 0x10;
    // if (ctrl & 0xff) {
    //     ESP_DRAM_LOGI(TAG, "TXB0CTRL %x", ctrl);
    // }

    // uint8_t eflg;
    // self->mcp25625.readRegister(MCP25625::reg::EFLG, eflg);
    // ESP_DRAM_LOGI(TAG, "TXB0CTRL %x", eflg);
}
