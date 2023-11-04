#pragma once

#include "esp_http_server.h"
#include "freertos/timers.h"

#include <unordered_map>
#include <functional>
#include <string>

class HttpServer {
public:
    HttpServer();
    virtual ~HttpServer() {}

    void start();
    void sendFrame(std::string frame);
    void sendFrame(uint8_t * data, size_t const length);
    bool isWebsocketConnected();

    // static because the callback it's called from has no this pointer
    static std::function<void(uint8_t * payload, size_t len)> onFrame;
    static std::function<void()> onConnectStatusChanged;
private:
    TimerHandle_t pingTimer;

    // todo make this instance variables if possible
    static httpd_handle_t server;
    static int socketFd;

    static void sendFile(httpd_req_t * req, std::string uri);
    static esp_err_t handleGetStatic(httpd_req_t *req);
    static esp_err_t handleWebSocket(httpd_req_t *req);
    esp_err_t handleWebsocketConnect(httpd_req_t *req);
    static std::string getMimeType(std::string path);
    static void pingFunction(TimerHandle_t xTimer);

    class PingPong {
    public:
        PingPong();

        void start();
        void stop();
        void sendPing();
        void receivedPong();
    private:
        uint16_t pingInterval = 1000;
        TimerHandle_t pingTimer;
        static void pingFunction(TimerHandle_t xTimer);

        uint16_t pendingPingCount;
        const uint16_t pendingPingCountMax = 3;
    };

    PingPong pingPong;

};
