#include "mcp25625.h"

#include "esp_log.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <inttypes.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS  15

#define PARALLEL_LINES 10

static char const * const TAG = "mcp25625";

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
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    spi_device_interface_config_t devcfg {
        .command_bits = 8,
        .address_bits = 8,
        .mode = 0,                                // either 0,0 or 1,1 for mcp25625
        .clock_speed_hz = 10*1000*1000,           // Clock out at 26 MHz
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,                          // We want to be able to queue 7 transactions at a time
    };

    esp_err_t err;
    const spi_host_device_t canHost = VSPI_HOST;
    err = spi_bus_initialize(canHost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(err);

    spi_device_handle_t spi;
    err = spi_bus_add_device(canHost, &devcfg, &spi);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "spi device configured");

    registerTest(spi);

    err = spi_bus_remove_device(spi);
    ESP_ERROR_CHECK(err);

    err = spi_bus_free(canHost);
    ESP_ERROR_CHECK(err);
}
void MCP25625::registerTest(spi_device_handle_t spi) {
    esp_err_t err;

    const uint8_t MCP_READ = 0x03;  
    spi_transaction_t readmcp {};
    readmcp.cmd = MCP_READ;
    readmcp.addr = 0x0f;
    readmcp.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    readmcp.length = 8;
    readmcp.rxlength = 8;
    err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    spi_transaction_t  * readmcpresult {};
    err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "read: %d %x %x %x %x", readmcpresult->rxlength, readmcpresult->rx_data[0],  readmcpresult->rx_data[1],  readmcpresult->rx_data[2],  readmcpresult->rx_data[3]);
}
