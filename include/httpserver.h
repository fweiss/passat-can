#pragma once

#include "esp_http_server.h"
#include <unordered_map>
#include <functional>

class HttpServer {
public:
    HttpServer();
    virtual ~HttpServer() {}

    void start();
    void sendFrame(std::string frame);
    // static?
    static std::function<void(uint8_t * payload, size_t len)> onFrame;
private:
    httpd_handle_t server;
    static void sendFile(httpd_req_t * req);
    static esp_err_t hello_get_handler(httpd_req_t *req);
    static esp_err_t handleWebSocket(httpd_req_t *req);
    static std::string getMimeType(std::string path);
    // fixme should it be static?
    static int fd;
};
