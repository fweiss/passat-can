#include "indicator.h"

Indicator::Indicator() : rgbLed() {
    rgbLed.init();

    xTaskCreate(task, "Indicator_Task", 4096, (void*)this, 10, &taskHandle);
}

Indicator * Indicator::instance{nullptr};

Indicator * Indicator::getInstance() {
    if (instance == nullptr) {
        instance = new Indicator();
    }
    return instance;
}

void Indicator::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    rgbLed.setColor(red, green, blue);
}

void Indicator::postState(IndicatorState state) {

}

void Indicator::task(void* args) {
    Indicator * self = static_cast<Indicator*>(args);

    while (true) {
        self->blink();
    }
}

void Indicator::blink() {
    rgbLed.setColor(100, 0, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    rgbLed.setColor(0,0,0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}