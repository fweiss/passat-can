#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mcp25625.h"

class Fuzzer {
public:
    Fuzzer(MCP25625 *mcp25625);
    virtual ~Fuzzer();

    static const uint32_t periodMillis = 100;

    void start();
    void stop();

private:
    MCP25625 *mcp25625;

    static void taskFunction(void* pvParameters);
    TaskHandle_t taskHandle;
    void fuzzingFunction();
};
