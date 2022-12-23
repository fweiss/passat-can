#include "mcp25625.h"

#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

static char const * const TAG = "mcp25625";

MCP25625::MCP25625() {
    receiveQueue = xQueueCreate(10, sizeof(receive_msg_t));
}
MCP25625::~MCP25625() {
}
void MCP25625::init() {
    SPI::init();
    reset();
}

void MCP25625::deinit() {
    SPI::deinit();
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

    // it seems to take a bit of time for the driver to set all the pins
    // vTaskDelay(10 / portTICK_PERIOD_MS);

    while (false) {
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
static QueueHandle_t *gg;

void MCP25625::attachReceiveInterrupt() {
    esp_err_t err;

    ESP_LOGI(TAG, "attaching receive interrupt");

    // err = esp_intr_alloc(source, flags, handler, arg, &receiveInterruptHandle);
    // gpio_install_isr_service

    const gpio_num_t interruptPin = GPIO_NUM_21;
    gpio_pad_select_gpio(interruptPin);
    gpio_set_direction(interruptPin, GPIO_MODE_INPUT);
    gpio_pulldown_dis(interruptPin);
    gpio_pullup_en(interruptPin);
    gpio_set_intr_type(interruptPin, GPIO_INTR_NEGEDGE);
gg = &receiveQueue;
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    err = gpio_isr_handler_add(interruptPin, receiveInterruptISR, this);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "give this %p", this);
    } else {
        ESP_LOGE(TAG, "ist handler add err: %x", err);
    }
}
void MCP25625::detachReceiveInterrupt() {
    esp_err_t err;
    err = esp_intr_free(receiveInterruptHandle);
}

// a little adapter to dispatch methods from a static ISR
// void MCP25625::receiveInterruptISR(void *arg) {
//     // ESP_LOGI(TAG, "got this %p", arg);
//     // capture object pointer from opaque arg
//     MCP25625 *self = static_cast<MCP25625*>(arg);
//     if (self != NULL) {
//         gg->receiveDataEnqueue();
//     }
// }
void IRAM_ATTR MCP25625::receiveInterruptISR(void *arg) {
    receive_msg_t message;
    BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
    xQueueSendFromISR(*gg, &message, &xHigherPriorityTaskWokenByPost);
    if (xHigherPriorityTaskWokenByPost) {
        portYIELD_FROM_ISR();
    }
}

bool MCP25625::receiveMessage(receive_msg_t & message) {
    uint8_t canintf;
    readRegister(reg::CANINTF, canintf);
    if (canintf & 0x01) {
        uint8_t rxb0dlc;
        readRegister(reg::RXB0DLC, rxb0dlc);
        uint8_t dlc = rxb0dlc & 0x0f;
        uint8_t rxbosidh;
        readRegister(reg::RXB0SIDH, rxbosidh);
        uint8_t rxbosidl;
        readRegister(reg::RXB0SIDL, rxbosidl);
        uint16_t sid = (rxbosidh << 3) | ((rxbosidl & 0xe0) >> 5);

        message.identifier = sid;
        message.data_length_code = dlc;
        // todo read all the bytes
        readRegister(reg::RXB0D0, message.data[0]);

        // do this here instead of ISR - assume internal FIFO
        bitModifyRegister(reg::CANINTF, 0x01, 0x00); // clear rxb0

        return true;
    }
    return false;
}

void MCP25625::testReceiveStatus() {
    uint8_t canintf;
    readRegister(reg::CANINTF, canintf);
    uint8_t caninte;
    readRegister(reg::CANINTE, caninte);
    uint8_t eflg;
    readRegister(reg::EFLG, eflg);
    ESP_LOGI(TAG, "receive status cantf: %x caninte: %x eflg %x", canintf, caninte, eflg);
}
