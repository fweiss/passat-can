#pragma once

#include "app.h"
#include "mcp25625.h"

class AppMcp25625 : public App {
public:
    AppMcp25625();
    virtual ~AppMcp25625();

    void initBridge() override;
    void startBridge() override;

private:
    MCP25625 mcp25625;

    QueueHandle_t webSocketSendQueue;
    static void webSocketSendTask(void * pvParameters);

    TaskHandle_t canReceiveFrameTask;
    static void canReceiveFrameTaskFunction(void * pvParameters);

    TaskHandle_t canTransmitFrameTask;
    static void canTransmitFrameTaskFunction(void * pvParameters);
    
    TaskHandle_t canErrorTask;
    static void canErrorTaskFunction(void * pvParameters);

    TimerHandle_t heartbeatTimer;
    static void heartbeatFunction(tmrTimerControl*);
    TimerHandle_t canStatusTimer;
    static void canStatusFunction(TimerHandle_t xTimer);

    static void fuzzingFunction(TimerHandle_t xTimer);
    TimerHandle_t fuzzingTimer;
};
