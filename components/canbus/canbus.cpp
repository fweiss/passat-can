#include "canbus.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/twai.h"

static char const * const TAG = "CAN";

// #ifdef ESP32_S3
// // s3 does not define GPIO_22
// const gpio_num_t tx_io_num = GPIO_NUM_21;
// const gpio_num_t rx_io_num = GPIO_NUM_20;
// #else
// const gpio_num_t tx_io_num = GPIO_NUM_21;
// const gpio_num_t rx_io_num = GPIO_NUM_22;
// #endif

const gpio_num_t tx_io_num = GPIO_NUM_21;
const gpio_num_t rx_io_num = GPIO_NUM_22;

CanBus::CanBus() {
     recvCallback = [] (twai_message_t & message) {};
 }

void CanBus::init() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_io_num, rx_io_num, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    g_config.tx_queue_len = 20; // did not solve "could not decode a text frame as utf-8"
    // f_config.acceptance_code = (0x320 << 21);
    // f_config.acceptance_mask = ~(0x7ff << 21);

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver installed");
    } else {
        ESP_LOGE(TAG, "Failed to install driver");
        return;
    }

    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Driver started");
    } else {
        ESP_LOGE(TAG, "Failed to start driver");
        return;
    }
   
    //Wait for message to be received
    twai_message_t message;
    if (twai_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK) {
        ESP_LOGI(TAG, "Message received");
    } else {
        ESP_LOGE(TAG, "Failed to receive message");
        return;
    }

    recvCallback(message);

    // //Process received message
    // if (message.flags & CAN_MSG_FLAG_EXTD) {
    //     ESP_LOGI(TAG, "Message is in Extended Format");
    // } else {
    //     ESP_LOGI(TAG, "Message is in Standard Format");
    // }
    // ESP_LOGI(TAG, "ID is %d", message.identifier);
    // if (!(message.flags & CAN_MSG_FLAG_RTR)) {
    //     for (int i = 0; i < message.data_length_code; i++) {
    //         ESP_LOGI(TAG, "Data byte %d = %d", i, message.data[i]);
    //     }
    // }

}

void CanBus::onRecvFrame(std::function<void(twai_message_t & message)> callback) {
    recvCallback = callback;
}

void CanBus::sendFrame(twai_message_t & message) {
    esp_err_t err;
    err = twai_transmit(&message, pdMS_TO_TICKS(1000));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "tramsit err: %x", err);
    }
}

void CanBus::triggerRead() {
    //Wait for message to be received
    twai_message_t message;
    esp_err_t err = twai_receive(&message, pdMS_TO_TICKS(10000));
    if (err == ESP_OK) {
        // ESP_LOGI(TAG, "Message received");
    } else {
        ESP_LOGE(TAG, "receive err: %x", err);
        return;
    }
    recvCallback(message); 
}

std::string CanBus::messageToString(twai_message_t & message) {
    char buf[20];
    snprintf(buf, sizeof(buf), "0x%x (%d)", message.identifier, message.identifier);
    std::string out(buf);
    for (int i = 0; i < message.data_length_code; i++) {
        snprintf(buf, sizeof(buf), " %0x", message.data[i]);
        out += std::string(buf);
    }
    return out;
}
