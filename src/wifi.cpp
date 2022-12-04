#include "wifi.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "wifi";

WiFi::WiFi() {

}

WiFi::~WiFi() {

}

void WiFi::startStation() {
    ESP_LOGI(TAG, "starting wifi station mode");

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    registerAnyId();
    registerGotIp();

    wifi_config_t wifi_config{}; // important init to zero
    strcpy((char*)wifi_config.sta.ssid, CONFIG_ESP_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_ESP_WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    waitForConnection();
}

void WiFi::startAccessPoint() {
    ESP_LOGI(TAG, "starting wifi access point mode");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap(); // ap instead of sta

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    const char staSSID[] = "wallop";
    const char staPASS[] = "";
    uint8_t staChannel = 5;
    uint8_t staMaxConnections = 5;
    // wifi_config_t wifi_configx = {
    //     .ap = {
    //         .ssid = staSSID,
    //         .ssid_len = strlen(staSSID),
    //         .channel = staChannel,
    //         .password = staPASS,
    //         .max_connection = staMaxConnections,
    //         .authmode = WIFI_AUTH_WPA_WPA2_PSK,
    //         .pmf_cfg = {
    //                 .required = false,
    //         },
    //     },
    // };
    wifi_config_t wifi_config{};
    wifi_ap_config_t & ap = wifi_config.ap;
    memcpy(&ap.ssid, &staSSID, sizeof(staSSID));
    ap.ssid_len = strlen(staSSID);
    ap.channel = staChannel;
    memcpy(&ap.password, &staPASS, sizeof(staPASS));
    ap.max_connection = staMaxConnections;
    ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    // ap.pmf_cfg.required = fales;
    
    if (strlen(staPASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             "FIXME", "FIXME", 5);
}

void WiFi::registerAnyId() {
    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        (void*)this,
                                                        &instance_any_id));
}

void WiFi::registerGotIp() {
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        (void*)this,
                                                        &instance_got_ip));
}

void WiFi::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WiFi *self = static_cast<WiFi*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            self->s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(self->s_wifi_event_group, self->WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        self->s_retry_num = 0;
        xEventGroupSetBits(self->s_wifi_event_group, self->WIFI_CONNECTED_BIT);
    }
    // maybe should be separate handler?
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station connected");
        // ESP_LOGI(TAG, "station %02x:%02x:%02x:%02x:%02x:%02x %s join, AID=%d",
        //          MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station disconnected");
        // ESP_LOGI(TAG, "station %02x:%02x:%02x:%02x:%02x:%02x %s leave, AID=%d",
        //          MAC2STR(event->mac), event->aid);
    }
}

void WiFi::waitForConnection() {
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
