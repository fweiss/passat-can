#include "indicator.h"

Indicator::Indicator() : rgbLed(), state(IndicatorState::init) {
    rgbLed.init();

    red = 100;
    green = 0;
    blue = 0;

    xTaskCreate(task, "Indicator_Task", 4096, (void*)this, 10, &taskHandle);
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
    // if (state == stationConnecting) {
    //     red = 0;
    //     green = 100;
    //     blue = 0;
    //     return;
    // }
    // red = 0;
    // green = 0;
    // blue = 100;
    this->state = state;
}

void Indicator::task(void* args) {
    Indicator * self = static_cast<Indicator*>(args);

    const Color wifiColor{0,0,100};

    while (true) {
        // self->blink(Color{100, 0, 0});
        // self->pulse(Color{0, 100, 0}, 3);
        if (self->state == accessPointConnecting) {
            self->pulse(wifiColor, 2);
        } else if (self->state == stationConnecting) {
            self->pulse(wifiColor, 3);
        } else if (self->state == wifiConnected) {
            self->blink(wifiColor);
        }
        // idle
        vTaskDelay(500 / portTICK_PERIOD_MS);
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