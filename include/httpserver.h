#pragma once

#include "esp_http_server.h"

class HttpServer {
public:
    HttpServer();
    virtual ~HttpServer() {}

    void start();
private:
    httpd_handle_t server;
    static void sendFile(httpd_req_t * req);
    static esp_err_t hello_get_handler(httpd_req_t *req);
    static esp_err_t handleWebSocket(httpd_req_t *req);
};
