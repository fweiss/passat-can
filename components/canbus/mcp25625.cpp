#include "mcp25625.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <inttypes.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
#include "driver/spi_master.h"

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19

#define PARALLEL_LINES 10

MCP25625::MCP25625() {

}
MCP25625::~MCP25625() {
}
void MCP25625::init() {
    spi_bus_config_t buscfg {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES*320*2+8,
    };
}
