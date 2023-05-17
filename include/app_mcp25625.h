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

    static void webSocketSendTask(void * pvParameters);
    TimerHandle_t heartbeatTimer;
    static void heartbeatFunction(tmrTimerControl*);
};
