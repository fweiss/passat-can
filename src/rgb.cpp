#include "rgb.h"

RGB::RGB() {}

void RGB::init() {

    
    const int RMT_LED_STRIP_RESOLUTION_HZ = 10000000; // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
    const int RMT_LED_STRIP_GPIO_NUM      = 48;

    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        // .flags.invert_out = false,
        // .flags.with_dma = false,
        // .flags.invert_out = false,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = {
            .duration0 = static_cast<unsigned int>(0.3 * tx_chan_config.resolution_hz / 1000000), // T0H=0.3us
            .level0 = 1,
            .duration1 = static_cast<unsigned int>(0.9 * tx_chan_config.resolution_hz / 1000000), // T0L=0.9us
            .level1 = 0,
        },
        .bit1 = {
            .duration0 = static_cast<unsigned int>(0.9 * tx_chan_config.resolution_hz / 1000000), // T1H=0.9us
            .level0 = 1,
            .duration1 = static_cast<unsigned int>(0.3 * tx_chan_config.resolution_hz / 1000000), // T1L=0.3us
            .level1 = 0,
        },
        .flags = {
            .msb_first = 0 // WS2812 transfer bit order: G7...G0R7...R0B7...B0
        },
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoder_config, &led_encoder_handle));

    ESP_ERROR_CHECK(rmt_enable(led_chan));

}

void RGB::setColor() {
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    // note WS2812 is GRB
    led_pixels[0] = 100;
    led_pixels[1] = 0; // r?
    led_pixels[2] = 0;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder_handle, led_pixels, sizeof(led_pixels), &tx_config));
}
