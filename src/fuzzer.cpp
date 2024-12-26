#include "fuzzer.hpp"
#include "esp_log.h"

static const char TAG[] = "Fuzzer";

Fuzzer::Fuzzer(MCP25625 *mcp25625) : mcp25625(mcp25625) {
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
        fuzzer->fuzzingFunction();
        vTaskDelay(periodMillis / portTICK_PERIOD_MS);
    }
}

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
    ESP_DRAM_LOGI(TAG, "fuzzingFunction %lx", canFrame.identifier);
    mcp25625->transmitFrame(canFrame);
    // canFrame.identifier += 1;

    uint8_t ctrl;
    while(true) {
        mcp25625->readRegister(MCP25625::reg::TXB0CTRL, ctrl);
        if ((ctrl & 0x08) == 0) {
            break;
        }
    }
    const uint8_t TXERR = 0x10;
    if (ctrl & 0xff) {
        ESP_DRAM_LOGI(TAG, "TXB0CTRL %x", ctrl);
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
