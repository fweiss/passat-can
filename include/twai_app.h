#pragma once

#include "app.h"
#include "canbus.h"

class TwaiApp : public App {
public:
    TwaiApp();
    void initBridge() override;

    void start();

private:
    CanBus canbus;
};

class McpApp : public App {
public:
    void initBridge() override;
};
