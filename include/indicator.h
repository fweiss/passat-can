#pragma once

#include "rgb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * Show connection status
 * wifi station/access point
 * wifi link connect
 * websocket connect
 * canbus activity
 */

class Indicator {
public:
    enum IndicatorState {
        wifiConnected,
        stationConnected,
    };
    struct Color {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    Indicator();
    virtual ~Indicator() {};

    static Indicator * getInstance();

    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void setColor(const Color & color);
    void postState(IndicatorState state);

private:
    RGB rgbLed;
    static Indicator * instance;
    TaskHandle_t taskHandle;
    static void task(void* args);
    void blink(const Color & color);
    void pulse(const Color & color, uint16_t times);

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    const Color OFF{0,0,0};
};
