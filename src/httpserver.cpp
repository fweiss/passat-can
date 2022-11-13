#include "httpserver.h"

#include <string>
#include <unordered_map>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"

char const * const TAG = "HTTP";

//maybe use singleton instead of statics
httpd_handle_t HttpServer::server;
int HttpServer::fd = 0;
std::function<void(uint8_t * payload, size_t len)> HttpServer::onFrame = [] (uint8_t * payload, size_t len) {};

HttpServer::HttpServer() {
}

void HttpServer::start() {
    static const httpd_uri_t defaultOptions = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = hello_get_handler,
        .user_ctx  = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = NULL,
        .supported_subprotocol = NULL,
    };
    static const httpd_uri_t websocketOptions = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handleWebSocket,
        .user_ctx  = NULL,
        .is_websocket = true,               // Mandatory: set to `true` to handler websocket protocol
        .handle_ws_control_frames = false,  // Optional: set to `true` for the handler to receive control packets, too
        .supported_subprotocol = NULL,
    };

    this->server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&this->server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &websocketOptions);
        httpd_register_uri_handler(server, &defaultOptions);
        // httpd_register_uri_handler(server, &echo);
        // httpd_register_uri_handler(server, &ctrl);
        #if CONFIG_EXAMPLE_BASIC_AUTH
        // httpd_register_basic_auth(server);
        #endif
        return;
    }

    ESP_LOGI(TAG, "Error starting server!");
    // return NULL;
}

esp_err_t HttpServer::hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    // const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    sendFile(req);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

esp_err_t HttpServer::handleWebSocket(httpd_req_t * req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "websocket handshake");
        fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "saved fd %d", fd);

        return ESP_OK;
    }

    // save socket descriptor for subsequent async sends
    fd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "saved fd %d", fd);

    startPingTimer();

    uint8_t buf[120];
    httpd_ws_frame_t ws_frame;
    ws_frame.type = HTTPD_WS_TYPE_TEXT;
    ws_frame.payload = buf;
    ws_frame.len = 0;

    httpd_ws_recv_frame(req, &ws_frame, sizeof(buf) - 1);
    if (ws_frame.len < sizeof(buf)) {
        buf[ws_frame.len] = 0;
    }
    ESP_LOGI(TAG, "received web socket frame: %s", ws_frame.payload);
    onFrame(ws_frame.payload, ws_frame.len);

    // esp_err_t ret = httpd_ws_send_frame(req, &ws_frame);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "ws send failed %d", ret);
    // }
    return ESP_OK;
}

std::string HttpServer::getMimeType(std::string path) {
    std::string suffix = path.substr(path.find_last_of(".") + 1);
    static std::unordered_map<std::string, std::string> getMimeType {
        { "html", "text/html" },
        { "js", "application/javascript; charset=utf-8" },
        { "css", "text/css" },
        { "ico", "image/ico" },
    };
    // or C++20 contains()
    try {
        return getMimeType.at(suffix);
    }
    catch (...) {
        return std::string("text/plain");
    }
}

void HttpServer::sendFrame(std::string data) {
    esp_err_t err;
    // httpd_ws_frame_t ws_pkt = {
    //     .final = true,
    //     .fragmented = false, 
    //     .type = HTTPD_WS_TYPE_BINARY, // was text
    //     .payload = (uint8_t *)data.c_str(),
    //     .len = data.size(),
    // };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt)); // clear to avoid errant flags but we're setting all the fields!
    ws_pkt.final = true;
    ws_pkt.fragmented = false;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = (uint8_t *)data.c_str();
    ws_pkt.len = data.size();

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, fd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing frame 0x%x %d", err, fd);
    }
}

void HttpServer::sendFrame(uint8_t * data, size_t const length) {
    esp_err_t err;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt)); // clear to avoid errant flags but we're setting all the fields!
    ws_pkt.final = true;
    ws_pkt.fragmented = false;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = data;
    ws_pkt.len = length;

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, fd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing frame 0x%x %d", err, fd);
    }    
}

bool HttpServer::isWebsocketConnected() {
    return fd != 0;
}

void HttpServer::sendFile(httpd_req_t * req) {
    std::string uri(req->uri);
    std::string filename = std::string("/spiffs") + uri;
    std::string mimeType = getMimeType(uri);

    (void)httpd_resp_set_type(req,  mimeType.c_str());

    char buf[128];
    ssize_t buf_len;

    // Check if destination file exists before renaming
    // struct stat st;
    // if (stat("/spiffs/foo.txt", &st) == 0) {
    //     // Delete it if it exists
    //     unlink("/spiffs/foo.txt");
    // }

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

void HttpServer::startPingTimer() {
    TickType_t const timerPeriod= pdMS_TO_TICKS(1000);
    bool const autoReload = true;
    TimerHandle_t handle = xTimerCreate("ping timer", timerPeriod, autoReload, nullptr, HttpServer::pingFunction);
    if (handle == NULL) {
        ESP_LOGE(TAG, "ping timer create error");
    } else {
        BaseType_t err = xTimerStart(handle, 0);
        if (err != pdPASS) {
            ESP_LOGE(TAG, "start ping timer failed");
        }
    }
}

void HttpServer::pingFunction(void*) {
    esp_err_t err;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt)); // clear to avoid errant flags but we're setting all the fields!
    ws_pkt.final = true;
    ws_pkt.fragmented = false;
    ws_pkt.type = HTTPD_WS_TYPE_PING;
    ws_pkt.payload = nullptr;
    ws_pkt.len = 0;

    // async does not require the httpd_req_t
    err = httpd_ws_send_frame_async(server, fd, &ws_pkt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error enqueuing ping frame 0x%x", err);
    } else {
        ESP_LOGI(TAG, "sent ping frame");
    }
}
