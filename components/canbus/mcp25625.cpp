#include "mcp25625.h"

#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// todo convert these to cont int
#define ESP32_S3_DEVKIT
#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS  10
#ifdef ESP32_S3_DEVKIT
#elif
#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK  14
#define PIN_NUM_CS  15
#endif

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
    const spi_host_device_t canHost = SPI3_HOST;
    err = spi_bus_initialize(canHost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(err);

    err = spi_bus_add_device(canHost, &devcfg, &spi);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "spi device configured");

    // testRegisters();
    testReceive();
    // testLoopBack();

    err = spi_bus_remove_device(spi);
    ESP_ERROR_CHECK(err);

    err = spi_bus_free(canHost);
    ESP_ERROR_CHECK(err);
}

void MCP25625::readRegister(uint8_t const address, uint8_t & value) {
    esp_err_t err;

    spi_transaction_t readmcp {};
    readmcp.cmd = cmd::READ;
    readmcp.addr = address;
    readmcp.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // | SPI_TRANS_MODE_OCT;
    readmcp.length = 8;
    readmcp.rxlength = 8;

    // err = spi_device_transmit(spi, &readmcp);
    // ESP_ERROR_CHECK(err);
    // value = readmcp.rx_data[0];

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

    // // spi_device_transmit is only about 10% faster
    // err = spi_device_transmit(spi, &readmcp);
    // ESP_ERROR_CHECK(err);

    err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    spi_transaction_t  * readmcpresult {};
    err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
    // ESP_LOGI(TAG, "read: %d %x %x %x %x", readmcpresult->rxlength, readmcpresult->rx_data[0],  readmcpresult->rx_data[1],  readmcpresult->rx_data[2],  readmcpresult->rx_data[3]);
}

void MCP25625::testRegisters() {
    ESP_LOGI(TAG, "starting register test");

    reset();

    uint8_t value;
    readRegister(reg::CANCTRL, value);
    ESP_LOGI(TAG, "register %x is %x", CANCTRL, value);

    uint8_t v2;
    writeRegister(reg::CANINTF, 0x11);
    readRegister(reg::CANINTF, v2);
    ESP_LOGI(TAG, "read back %x", v2);

    SJW sjw(7);
    // bitModifyRegister(CNF1, 0x01, 0x00);
    bitModifyRegister(reg::CNF1, sjw);
}

void MCP25625::testReceive() {
    ESP_LOGI(TAG, "starting receive test");
    reset();
    timing();
    bitModifyRegister(reg::RXB0CTRL, 0x60, 0x60); // receive any message
    bitModifyRegister(reg::CANINTE, 0x01, 0x01); // todo abstract
    REQOP normalMode(0); // get out of configuration mode
    bitModifyRegister(reg::CANCTRL, normalMode);
    while (true) {
        // uint8_t rec;
        // uint8_t eflg;
        // readRegister(reg::REC, rec);
        // readRegister(reg::EFLG, eflg);
        // ESP_LOGI(TAG, "rx error counter: %x %x", rec, eflg);

        uint8_t canintf;
        readRegister(reg::CANINTF, canintf);
        bool rxb0 = (canintf & 0x01); // check rb0 interrupt
        if (rxb0) {
            uint8_t value;
            bitModifyRegister(reg::CANINTF, 0x01, 0x00); // clear rxb0
            readRegister(reg::RXB0DLC, value);
            ESP_LOGI(TAG, "dlc %x", value);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void MCP25625::bitModifyRegister(uint8_t const address, uint8_t const mask, uint8_t value) {
    esp_err_t err;

    uint8_t data[2] = { mask, value };

    spi_transaction_t readmcp {};
    readmcp.cmd = cmd::BIT_MODIFY;
    readmcp.addr = address;
    readmcp.length = 16;
    readmcp.rxlength = 0;
    readmcp.tx_buffer = &data;

    // // spi_device_transmit is only about 10% faster
    // err = spi_device_transmit(spi, &readmcp);
    // ESP_ERROR_CHECK(err);

    err = spi_device_queue_trans(spi, &readmcp, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    spi_transaction_t  * readmcpresult {};
    err = spi_device_get_trans_result(spi, &readmcpresult, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
}


void MCP25625::bitModifyRegister(uint8_t address, FieldValue f) {
    bitModifyRegister(address, f.mask, f.bits);
}

void MCP25625::reset() {
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

void MCP25625::timing() {
    SJW sjw(3); // code 3 = 4
    BRP brp(0);
    SAM sam(0);
    PHSEG1 phseg1(5);
    PRSEG prseg(6);
    PHSEG2 phseg2(5);

    bitModifyRegister(reg::CNF1, sjw);
    bitModifyRegister(reg::CNF1, brp);
    bitModifyRegister(reg::CNF2, sam);
    bitModifyRegister(reg::CNF2, phseg1);
    bitModifyRegister(reg::CNF2, prseg);
    bitModifyRegister(reg::CNF3, phseg2);

    // writeRegister(reg::CNF1, 0x40);
    // writeRegister(reg::CNF2, 0xf6);
    // writeRegister(reg::CNF3, 0x84);
}

void MCP25625::testLoopBack() {
    ESP_LOGI(TAG, "starting loopback test");

    reset();
    timing();

    // setup
    // clear TXREQ?
    // transmit priority>?
    REQOP reqop(2); // loopback
    bitModifyRegister(reg::CANCTRL, reqop);

    // transmit
    writeRegister(reg::TXB0SIDH, 0x01);
    writeRegister(reg::TXB0SIDL, 0x02);
    writeRegister(reg::TXB0DLC, 0x00);
    writeRegister(reg::TXB0CTRL, 0x04); //0x04 = TXREQ

    // check receive
    uint8_t canintf;
    for (int i=1; i<10; i++) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        readRegister(reg::CANINTF, canintf);
        ESP_LOGI(TAG, "loopback canintf %x", canintf);
    }
}

void MCP25625::attachReceiveInterrupt() {
    esp_err_t err;
    // err = esp_intr_alloc(source, flags, handler, arg, &receiveInterruptHandle);

    receiveQueue = xQueueCreate(10, sizeof(int));

    const gpio_num_t interruptPin = GPIO_NUM_21;

    gpio_install_isr_service(0);
    gpio_isr_handler_add(interruptPin, receiveInterruptISR, this);

}
void MCP25625::detachReceiveInterrupt() {
    esp_err_t err;
    err = esp_intr_free(receiveInterruptHandle);
}
void IRAM_ATTR MCP25625::receiveInterruptISR(void *arg) {
    // check int flag
    // get message buffer
    // clear in flag
    receive_msg_t message;
    xQueueSendFromISR(static_cast<MCP25625*>(arg)->receiveQueue, &message, NULL);
}
