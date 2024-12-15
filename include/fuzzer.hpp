#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Fuzzer {
public:
    Fuzzer();
    virtual ~Fuzzer();

    void start();
    void stop();

private:
    static void taskFunction(void* pvParameters);
    TaskHandle_t taskHandle;
    void fuzzingFunction();
};
