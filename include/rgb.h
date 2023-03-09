#pragma once

#include "driver/rmt_tx.h"

class RGB {
public:
    RGB();

    void init();

    void setColor();
private:
    rmt_channel_handle_t led_chan = NULL;
    rmt_encoder_handle_t led_encoder_handle;
    uint8_t led_pixels[1 * 3];
};
