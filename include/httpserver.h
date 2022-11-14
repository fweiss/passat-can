#pragma once

#include "esp_http_server.h"
#include "freertos/timers.h"

#include <unordered_map>
#include <functional>

class HttpServer {
public:
    HttpServer();
    virtual ~HttpServer() {}

    void start();
    void sendFrame(std::string frame);
    void sendFrame(uint8_t * data, size_t const length);
    // static?
    static std::function<void(uint8_t * payload, size_t len)> onFrame;
    bool isWebsocketConnected();
private:
    static httpd_handle_t server;
    static int socketFd;

    static void sendFile(httpd_req_t * req);
    static esp_err_t handleGetStatic(httpd_req_t *req);
    static esp_err_t handleWebSocket(httpd_req_t *req);
    static std::string getMimeType(std::string path);
    static void startPingTimer();
    static void pingFunction(void*);
};
