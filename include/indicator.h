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
        init,
        accessPointConnecting,
        stationConnecting,
        wifiConnected,
        canbusHeartbeat,
        canbusNoHeartbeat,
        websocketNotConnected,
        websocketConnected,
    };
    struct Color {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };
    struct Channel {
        Color color;
        IndicatorState state;
        uint8_t pulses;
    };

    Indicator();
    virtual ~Indicator() {};
    static Indicator * getInstance();

    void postState(IndicatorState state);

private:
    static Indicator * instance;

    TaskHandle_t taskHandle;
    static void taskCode(void* args);

    Channel channels[3];

    RGB rgbLed;
    void blink(const Color & color);
    void pulse(const Color & color, uint16_t times);
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void setColor(const Color & color);

    const Color OFF{0,0,0};
};
