#include "spi.h"

#include "esp_log.h"

static char const * const TAG = "mcp25625-spi";

// todo convert these to cont int
#define ESP32_S3_DEVKIT
#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS  10 // fixme s/b 15?
#ifdef ESP32_S3_DEVKIT
#elif
#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS  15
#endif

const int spi_clock_speed = 10*1000*1000;

void SPI::init() {
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
    err = spi_bus_initialize(canHost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(err);

    err = spi_bus_add_device(canHost, &devcfg, &spi);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "spi device configured");
}

void SPI::deinit() {
    esp_err_t err;

    err = spi_bus_remove_device(spi);
    ESP_ERROR_CHECK(err);

    err = spi_bus_free(canHost);
    ESP_ERROR_CHECK(err);
}

void SPI::readRegister(uint8_t const address, uint8_t & value) {
    esp_err_t err;

    spi_transaction_t transaction {};
    transaction.cmd = cmd::READ;
    transaction.addr = address;
    transaction.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    transaction.length = 8;
    transaction.rxlength = 8;

    err = spi_device_transmit(spi, &transaction);
    ESP_ERROR_CHECK(err);
    value = transaction.rx_data[0];
}

void SPI::writeRegister(uint8_t const address, uint8_t const value) {
    esp_err_t err;

    spi_transaction_t transaction {};
    transaction.cmd = cmd::WRITE;
    transaction.addr = address;
    transaction.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    transaction.length = 8;
    transaction.rxlength = 0;
    transaction.tx_data[0] = value;

    err = spi_device_transmit(spi, &transaction);
    ESP_ERROR_CHECK(err);
}

void SPI::bitModifyRegister(uint8_t const address, uint8_t const mask, uint8_t value) {
    esp_err_t err;

    uint8_t data[2] = { mask, value };

    spi_transaction_t transaction {};
    transaction.cmd = cmd::BIT_MODIFY;
    transaction.addr = address;
    transaction.length = 16;
    transaction.rxlength = 0;
    transaction.tx_buffer = &data;

    err = spi_device_transmit(spi, &transaction);
    ESP_ERROR_CHECK(err);
}

void SPI::bitModifyRegister(uint8_t address, FieldValue f) {
    bitModifyRegister(address, f.mask, f.bits);
}

void SPI::reset() {
    esp_err_t err;

    // using transaction_ext to set address length zero
    spi_transaction_ext_t transaction_ext {};
    spi_transaction_t & transaction = transaction_ext.base;
    transaction.cmd = cmd::RESET;
    transaction.flags = SPI_TRANS_VARIABLE_ADDR;
    transaction.length = 0;
    transaction.rxlength = 0;
    transaction_ext.address_bits = 0;

    err = spi_device_transmit(spi, &transaction);
    ESP_ERROR_CHECK(err); 
}

void SPI::readArrayRegisters(uint8_t startAddress, uint8_t * data, uint8_t count) {
    esp_err_t err;

    spi_transaction_t transaction {};
    transaction.cmd = cmd::READ;
    transaction.addr = startAddress;
    transaction.flags = 0;
    transaction.rx_buffer = data;
    transaction.length = 8 * count;
    transaction.rxlength = 8 * count;

    err = spi_device_transmit(spi, &transaction);
    ESP_ERROR_CHECK(err);
}
