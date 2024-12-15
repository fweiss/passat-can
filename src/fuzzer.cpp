#include "fuzzer.hpp"
#include "esp_log.h"

Fuzzer::Fuzzer() {
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
    Fuzzer* fuzzer = (Fuzzer*)pvParameters;
    fuzzer->fuzzingFunction();
}

void Fuzzer::fuzzingFunction() {
    while (true) {
        ESP_LOGI("Fuzzer", "Fuzzing...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
