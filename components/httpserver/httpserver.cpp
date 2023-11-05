#include "httpserver.h"

#include <string>
#include <unordered_map>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"

char const * const TAG = "HTTP";

const httpd_ws_frame_t default_ws_frame = {
    .final = true,
    .fragmented = false, 
    .type = HTTPD_WS_TYPE_BINARY,
    .payload = NULL,
    .len = 0,
};

//maybe use singleton instead of statics
httpd_handle_t HttpServer::server;
int HttpServer::socketFd = 0;

std::function<void(uint8_t * payload, size_t len)> HttpServer::onFrame = [] (uint8_t * payload, size_t len) {};
std::function<void()> HttpServer::onConnectStatusChanged = [] () {};

HttpServer::HttpServer() {
}

void HttpServer::start() {

    TickType_t const timerPeriod= pdMS_TO_TICKS(1000);
    bool const autoReload = true;
    pingTimer = xTimerCreate("ping timer", timerPeriod, autoReload, (void*)this, pingFunction);
    if (pingTimer == NULL) {
        ESP_LOGE(TAG, "ping timer create error");
    }

    static const httpd_uri_t defaultOptions = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = handleGetStatic,
        .user_ctx  = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL,
    };
    static const httpd_uri_t websocketOptions = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handleWebSocket,
        .user_ctx  = (void*)this,
        .is_websocket = true,               // Mandatory: set to `true` to handler websocket protocol
        .handle_ws_control_frames = true,   // Optional: set to `true` for the handler to receive ping/pong/close frames
        .supported_subprotocol = NULL,
    };

    this->server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.open_fn = [] (httpd_handle_t hd, int sockfd) -> esp_err_t {
        // cannot tell if this connection is web socket yet
        // need to wait for web socket handshake in handleWebSocket()

        return ESP_OK;
    }; // httpd_open_func_t 
    config.close_fn = [] (httpd_handle_t hd, int sockfd) { // httpd_close_func_t
        httpd_ws_client_info_t client_info = httpd_ws_get_fd_info(hd, sockfd);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            ESP_LOGI(TAG, "websocket closed %d", sockfd);
            onConnectStatusChanged();
        }
    }; 

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    esp_err_t err;
    err = httpd_start(&this->server, &config);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error starting server!");
        return;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    // TODO check err
    err = httpd_register_uri_handler(server, &websocketOptions);
    err = httpd_register_uri_handler(server, &defaultOptions);

    onConnectStatusChanged();
}

esp_err_t HttpServer::handleGetStatic(httpd_req_t *req) {
    if (strcmp(req->uri, "/") == 0) {
        sendFile(req, "/index.html");
    } else {
        sendFile(req, req->uri);
    }
    return ESP_OK;
}

void HttpServer::sendFile(httpd_req_t * req, std::string uri) {
    std::string filename = std::string("/spiffs") + uri;
    std::string mimeType = getMimeType(uri);

    (void)httpd_resp_set_type(req,  mimeType.c_str());

    char buf[128];
    ssize_t buf_len;

    ESP_LOGI(TAG, "opening file %s", filename.c_str());
    esp_err_t err = ESP_OK;
    FILE * fp;
    fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "open failed %d", (int)fp);
    }
    
    do {
        buf_len = fread(&buf, 1, sizeof(buf), fp);
        err = httpd_resp_send_chunk(req, buf, buf_len);
    } while (buf_len > 0 && err == ESP_OK);
    ESP_LOGI(TAG, "sent file");

    fclose(fp);
}

std::string HttpServer::getMimeType(std::string path) {
    std::string suffix = path.substr(path.find_last_of(".") + 1);
    static std::unordered_map<std::string, std::string> getMimeType {
        { "html", "text/html" },
        { "js", "application/javascript; charset=utf-8" },
        { "css", "text/css" },
        { "ico", "image/ico" },
    };

    // C++11, or use C++20 contains()
    auto search = getMimeType.find(suffix);
    return (search == getMimeType.end())
        ? std::string("text/plain")
        : search->second; 
}

esp_err_t HttpServer::handleWebSocket(httpd_req_t * req) {
    HttpServer* self = static_cast<HttpServer*>(req->user_ctx);

    if (req->method == HTTP_GET) {
        return self->handleWebsocketConnect(req);
    } else {
        uint8_t buf[120];
        httpd_ws_frame_t ws_frame = default_ws_frame;
        ws_frame.payload = buf;
        ws_frame.len = 0;

        esp_err_t err = httpd_ws_recv_frame(req, &ws_frame, sizeof(buf) - 1);
        if (ws_frame.len < sizeof(buf)) {
            buf[ws_frame.len] = 0; // was for string
        }

        // ESP_LOGI(TAG, "received web socket frame: len:%d type:%d final:%d payload[0]:%x", ws_frame.len, ws_frame.type, ws_frame.final, ws_frame.payload[0]);
        switch (ws_frame.type) {
            case HTTPD_WS_TYPE_CONTINUE: // not supported
                break;
            case HTTPD_WS_TYPE_TEXT:
            case HTTPD_WS_TYPE_BINARY:
                onFrame(ws_frame.payload, ws_frame.len);
                break;
            case HTTPD_WS_TYPE_CLOSE:
                ESP_LOGI(TAG, "received websocket close frame");
                self->socketFd = 0;
                self->pingPong.stop();
                onConnectStatusChanged();
                break;
            case HTTPD_WS_TYPE_PING:
                // ESP_LOGI(TAG, "received websocket ping frame");
                break;
            case HTTPD_WS_TYPE_PONG:
                ESP_LOGI(TAG, "received websocket pong frame");
                self->pingPong.receivedPong();
                break;
          }

        return ESP_OK;
    }
}

esp_err_t HttpServer::handleWebsocketConnect(httpd_req_t * req) {
    // save socket descriptor for subsequent async sends
    socketFd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "websocket handshake opened %d", socketFd);
    onConnectStatusChanged();
    // (void)xTimerStart(pingTimer, 0);
    pingPong.start();
    return ESP_OK;
}

bool HttpServer::isWebsocketConnected() {
    return socketFd != 0;
}

void HttpServer::sendFrame(std::string data) {
    esp_err_t err;
    httpd_ws_frame_t ws_pkt = default_ws_frame;
    ws_pkt.payload = (uint8_t *)data.c_str();
    ws_pkt.len = data.size();

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, socketFd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing frame 0x%x %d", err, socketFd);
    }
}

void HttpServer::sendFrame(uint8_t * data, size_t const length) {
    esp_err_t err;
    httpd_ws_frame_t ws_pkt = default_ws_frame;
    ws_pkt.payload = data;
    ws_pkt.len = length;

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, socketFd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing frame 0x%x %d", err, socketFd);
    }    
}

void HttpServer::pingFunction(TimerHandle_t xTimer) {
    esp_err_t err;

    httpd_ws_client_info_t clientInfo = httpd_ws_get_fd_info(server, socketFd);
    ESP_LOGI(TAG, "sent ping frame %d", clientInfo);

    httpd_ws_frame_t ws_pkt = default_ws_frame;
    ws_pkt.type = HTTPD_WS_TYPE_PING;
    // ws_pkt.payload = (uint8_t *)"ping";
    // ws_pkt.len = 4;

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, socketFd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing ping frame 0x%x", err);
    } else {
        // ESP_LOGI(TAG, "sent ping frame");
    }
}

/* ===================== PINGPONG ============== */

HttpServer::PingPong::PingPong() {
    // todo maybe allocate statically
    // using pvTimerID to refer to this
    const TickType_t timerPeriodMs = 2000;
    pingTimer = xTimerCreate("pingPongTimer", pdMS_TO_TICKS(timerPeriodMs), pdTRUE, (void *)this, pingFunction);
}

void HttpServer::PingPong::start() {
    pendingPingCount = 0;
    xTimerStart(pingTimer, 0);
}

void HttpServer::PingPong::stop() {
    xTimerStop(pingTimer, 0);
}

void HttpServer::PingPong::pingFunction(TimerHandle_t xTimer) {
    HttpServer::PingPong * self = (HttpServer::PingPong *)pvTimerGetTimerID(xTimer);

    self->sendPing();

    httpd_ws_client_info_t clientInfo = httpd_ws_get_fd_info(server, socketFd);
    ESP_LOGI(TAG, "ping fd info: %d", clientInfo);

    httpd_ws_frame_t ws_pkt = default_ws_frame;
    ws_pkt.type = HTTPD_WS_TYPE_PING;
    // ws_pkt.payload = (uint8_t *)"ping";
    // ws_pkt.len = 4;

    esp_err_t err;
    err = httpd_ws_send_frame_async(server, socketFd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing ping frame 0x%x", err);
    } else {
        // ESP_LOGI(TAG, "sent ping frame");
    }
}

void HttpServer::PingPong::sendPing() {
    ESP_LOGI(TAG, "ping pending: %d", pendingPingCount);
    pendingPingCount += 1;
    if (pendingPingCount > pendingPingCountMax) {
        // disconnect
    }
}

void HttpServer::PingPong::receivedPong() {
    ESP_LOGI(TAG, "pong pending: %d", pendingPingCount);
    if (pendingPingCount > 0) { // prevent underflow
        pendingPingCount -= 1;
    }
}
