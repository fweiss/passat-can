#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mcp25625.h"

class KeepAlive {
public:
    KeepAlive(MCP25625 *mcp25625, CanFrame canFrame, uint32_t periodms);
    KeepAlive(MCP25625 *mcp25625);
    virtual ~KeepAlive();

    static void simulatedGateway(MCP25625 *mcp25625);

    void start();
    void stop();
private:
    MCP25625 *mcp25625;
    CanFrame canFrame;
    uint32_t periodMillis;

    static void taskFunction(void* pvParameters);
    TaskHandle_t taskHandle;
};
