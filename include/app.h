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

protected:
    WiFi wifi;
    HttpServer httpServer;

private:
    void initNvs();
    void initSpiffs();
    void initWifi();
    void initHttpServer();
    virtual void initBridge();

};
