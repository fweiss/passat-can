#include "indicator.h"

const char * TAG = "indicator";

Indicator::Indicator() : rgbLed() {
    rgbLed.init();

    for (Channel & channel : channels) {
        channel.state = init;
        channel.pulses = 0;
    }
    channels[0].color = Color{0,0,10}; // wifi
    channels[1].color = Color{0,10,0}; // canbus
    channels[2].color = Color{10,10,10}; // websocket

    xTaskCreate(taskCode, "Indicator_Task", 4096, (void*)this, 10, &taskHandle);
}

Indicator * Indicator::instance{nullptr};

Indicator * Indicator::getInstance() {
    if (instance == nullptr) {
        instance = new Indicator();
    }
    return instance;
}

// @deprecated
void Indicator::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    rgbLed.setColor(red, green, blue);
}

void Indicator::setColor(const Color & color) {
    rgbLed.setColor(color.red, color.green, color.blue);
}

void Indicator::postState(IndicatorState state) {
    // this->state = state;

    switch (state) {
        case wifiConnected:
            channels[0].pulses = 1;
            break;
        case stationConnecting:
            channels[0].pulses = 2;
            break;
        case accessPointConnecting:
            channels[0].pulses = 3;
            break;

        case canbusHeartbeat:
            channels[1].pulses = 1;
            break;
        case canbusNoHeartbeat:
            channels[1].pulses = 2;
            break;

        case websocketConnected:
            channels[2].pulses = 1;
            break;
        case websocketNotConnected:
            channels[2].pulses = 2;
            break;

        case init:
            break;
    }
}

void Indicator::taskCode(void* args) {
    Indicator * self = static_cast<Indicator*>(args);

    // const Color wifiColor{0,0,100};
    // const Color canbusColor{100, 0, 0}; // trying to get orange, green too bright
    // const Color canbusColor{0, 10, 0};

    while (true) {
        uint16_t pulses = 0;
        for (Channel & channel : self->channels) {
            self->pulse(channel.color, channel.pulses);
            pulses += channel.pulses;
        }
        // to keep WDT from kicking
        if (pulses == 0) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void Indicator::blink(const Color & color) {
    setColor(color);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    setColor(OFF);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

/**
 * generate a sequence of short pulses
 * P_P_P_IIIII
 * P and _ are equal time, sum period / 4
 * except when there's onbly one P
 * I sum is period / 2
 */
void Indicator::pulse(const Color & color, uint16_t times) {
    if (times < 1) return;
    if (times == 1) {
        blink(color);
        return;
    }

    const uint16_t period = 1000;
    const uint16_t subPeriod = times == 1 ? period : period / 4;
    for (int i=0; i<times; i++) {
        setColor(color);
        vTaskDelay(subPeriod / 2 / portTICK_PERIOD_MS);
        setColor(OFF);
        vTaskDelay(subPeriod / 2 / portTICK_PERIOD_MS);
    }
    setColor(OFF);
    vTaskDelay(period / 2 / portTICK_PERIOD_MS);
}