#pragma once

#include "wifi.h"
#include "httpserver.h"

class App {
public:
    App();
    virtual ~App();

    void init();
    void deinit();

    virtual void start();

    virtual void initBridge();
    virtual void startBridge();

    enum WifiModes {
        STATION,
        ACCESS_POINT,
    };

protected:
    WiFi wifi;
    HttpServer httpServer;
    WifiModes wifiMode;

private:
    void initNvs();
    void initSpiffs();
    void initWifi();
    void initHttpServer();

private:
    void initWifiModeSwitch();
};
