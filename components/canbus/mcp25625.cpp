#include "mcp25625.h"

#include "esp_log.h"

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS  15

static char const * const TAG = "mcp25625";

const int spi_clock_speed = 10*1000*1000;

MCP25625::MCP25625() {

}
MCP25625::~MCP25625() {
}
void MCP25625::init() {
    // default for .max_transfer_sz and .flags
    spi_bus_config_t buscfg {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_device_interface_config_t devcfg {
        .command_bits = 8,
        .address_bits = 8,
        .mode = 0,                                // either 0,0 or 1,1 for mcp25625
        .clock_speed_hz = spi_clock_speed,           // Clock out at 26 MHz
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,                          // We want to be able to queue 7 transactions at a time
    };

    esp_err_t err;
    const spi_host_device_t canHost = HSPI_HOST;
    err = spi_bus_initialize(canHost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(err);

    err = spi_bus_add_device(canHost, &devcfg, &spi);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "spi device configured");

    // registerTest(spi);
    const uint8_t CANCTRL = 0x0e;
    uint8_t value;
    readRegister(CANCTRL, value);
    ESP_LOGI(TAG, "register %x is %x", CANCTRL, value);

    const uint8_t CNF1 = 0x2a;
    writeRegister(CNF1, 0x11);
    uint8_t v2;
    readRegister(CNF1, v2);
    ESP_LOGI(TAG, "read back %x", v2);

    err = spi_bus_remove_device(spi);
    ESP_ERROR_CHECK(err);

    err = spi_bus_free(canHost);
    ESP_ERROR_CHECK(err);
}
// void MCP25625::registerTest(spi_device_handle_t spi) {
//     esp_err_t err;

//     const uint8_t MCP_READ = 0x03;  
//     spi_transaction_t readmcp {};
//     readmcp.cmd = MCP_READ;
//     readmcp.addr = 0x0f;
//     readmcp.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
//     readmcp.length = 8;
//     readmcp.rxlength = 8;
//     err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
//     ESP_ERROR_CHECK(err);

//     spi_transaction_t  * readmcpresult {};
//     err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
//     ESP_ERROR_CHECK(err);
//     ESP_LOGI(TAG, "read: %d %x %x %x %x", readmcpresult->rxlength, readmcpresult->rx_data[0],  readmcpresult->rx_data[1],  readmcpresult->rx_data[2],  readmcpresult->rx_data[3]);
// }

void MCP25625::readRegister(uint8_t const address, uint8_t & value) {
    esp_err_t err;

    spi_transaction_t readmcp {};
    readmcp.cmd = cmd::READ;
    readmcp.addr = address;
    readmcp.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    readmcp.length = 8;
    readmcp.rxlength = 8;
    err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    spi_transaction_t  * readmcpresult {};
    err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
    // ESP_LOGI(TAG, "read: %d %x %x %x %x", readmcpresult->rxlength, readmcpresult->rx_data[0],  readmcpresult->rx_data[1],  readmcpresult->rx_data[2],  readmcpresult->rx_data[3]);
    value = readmcpresult->rx_data[0];
}
void MCP25625::writeRegister(uint8_t const address, uint8_t const value) {
    esp_err_t err;

    spi_transaction_t readmcp {};
    readmcp.cmd = cmd::WRITE;
    readmcp.addr = address;
    readmcp.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    readmcp.length = 8;
    readmcp.rxlength = 0;
    readmcp.tx_data[0] = value;
    err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    // TODO try spi_device_transmit
    spi_transaction_t  * readmcpresult {};
    err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
    // ESP_LOGI(TAG, "read: %d %x %x %x %x", readmcpresult->rxlength, readmcpresult->rx_data[0],  readmcpresult->rx_data[1],  readmcpresult->rx_data[2],  readmcpresult->rx_data[3]);
}

