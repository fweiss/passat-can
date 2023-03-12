#pragma once

#include "rgb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Indicator {
public:
    enum IndicatorState {
        wifiConnected,
    };

    Indicator();
    virtual ~Indicator() {};

    static Indicator * getInstance();

    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void postState(IndicatorState state);

private:
    RGB rgbLed;
    static Indicator * instance;
    TaskHandle_t taskHandle;
    static void task(void* args);
    void blink();
};
