#include "mcp25625.h"

#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <string.h>

static char const * const TAG = "mcp25625";

// forward
static uint32_t getIdentifier(uint8_t sidh, uint8_t sidl, uint8_t eid8, uint8_t eid0);

MCP25625::MCP25625() {
    receiveISRQueue = xQueueCreate(10, sizeof(timestamp_t));
    receiveMessageQueue = xQueueCreate(20, sizeof(receive_msg_t));
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
    ESP_LOGI(TAG, "init can timing");
    SJW sjw(3); // code 3 = 4
    BRP brp(0);
    SAM sam(0);
    PHSEG1 phseg1(5);
    PRSEG prseg(6);
    PHSEG2 phseg2(5);

    // bitModifyRegister(reg::CNF1, sjw);
    // bitModifyRegister(reg::CNF1, brp);
    // bitModifyRegister(reg::CNF2, sam);
    // bitModifyRegister(reg::CNF2, phseg1);
    // bitModifyRegister(reg::CNF2, prseg);
    // bitModifyRegister(reg::CNF3, phseg2);

    // 20MHz_500kBPS
    writeRegister(reg::CNF1, 0x40);
    writeRegister(reg::CNF2, 0xf6);
    writeRegister(reg::CNF3, 0x84);

    // 20MHz_250kBPS
    // writeRegister(reg::CNF1, 0x41);
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

    ESP_LOGI(TAG, "attach receive interrupt");

    // task to read buffer separate from ISR
    xTaskCreatePinnedToCore(receiveMessageTask, "receive task", 2048, this, 2, NULL, APP_CPU_NUM);

    // err = esp_intr_alloc(source, flags, handler, arg, &receiveInterruptHandle);
    // gpio_install_isr_service

    const gpio_num_t interruptPin = GPIO_NUM_21;
    esp_rom_gpio_pad_select_gpio(interruptPin);
    gpio_set_direction(interruptPin, GPIO_MODE_INPUT);
    gpio_pulldown_dis(interruptPin);
    gpio_pullup_en(interruptPin);
    gpio_set_intr_type(interruptPin, GPIO_INTR_NEGEDGE);
gg = &receiveISRQueue;
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
    ESP_ERROR_CHECK(err); 
}

// no need to demux the interrupt by inpecting CANINTF
// since both receive flag interrupts are available
// this was causing RX0OVR because the interrupt service time
// (int to int clear) was taking 167 us. This was OK most of the
// time, but the 1550 fid had only two bytes, in which case
// the interrupt ocurred before the previous flag was cleared.
// can shorten interrupt service by anothe 52 us by using
// read rx buffer which clears the flag.
bool MCP25625::receiveMessage(receive_msg_t * message) {
    // alignment required by SPI DMA
    struct alignas(32) buf {
        // uint8_t ctrl;
        uint8_t sidh;
        uint8_t sidl;
        uint8_t eid8;
        uint8_t eid0;
        uint8_t dlc;
        uint8_t data[8];
        uint8_t canstat;
        uint8_t canctrl;
    } buf;
    readArrayRegisters(RXB0SIDH, (uint8_t*)&buf, sizeof(buf));

    // message->identifier = (buf.sidh << 3) | ((buf.sidl & 0xe0) >> 5);
    message->identifier = getIdentifier(buf.sidh, buf.sidl, buf.eid8, buf.eid0);
    message->flags.srr = SRR::of(buf.sidl);
    message->flags.ide = IDE::of(buf.sidl);
    message->data_length_code = buf.dlc & 0x0f;
    memcpy(message->data, buf.data, sizeof(buf.data));

    // do this here instead of ISR
    // better yet use READ RX BUFFER command
    bitModifyRegister(reg::CANINTF, 0x01, 0x00); // clear rxb0

    return true; // todo no longer needed
}

// deprecated
void MCP25625::sendMessage(uint8_t * payload, size_t len) {
    ESP_LOGI(TAG, "send message");
    struct alignas(32) buf {
        // uint8_t ctrl;
        uint8_t sidh; // sid[10:3]
        uint8_t sidl; // sid[2:0] ide eid[17:16]
        uint8_t eid8; // eid[15:8]
        uint8_t eid0; // eid[7:0]
        uint8_t dlc;    // rtr dlc[3:0]
        uint8_t data[8];
        uint8_t canstat; // operationmode, interrupr flags
        uint8_t canctrl;
    } buf;
    memset(&buf, 0, sizeof(buf));

    uint16_t sid = 0x0391;
    buf.sidh = (sid >> 3) & 0xff;
    buf.sidl = ((sid & 0x07) << 5);
    // unsigned char comfortUp[3] = { 85,128,0 }; //put windows up
    buf.dlc = 3;
    buf.data[0] = 85;
    buf.data[1] = 128;
    buf.data[2] = 0;

    bitModifyRegister(reg::CANCTRL, 0x08, 0x08); // one shot mode
    // caninte merre
    bitModifyRegister(reg::CANINTE, 0x54, 0x54); // TX0IE

    // todo
    // check if TXREQ is set
    // check if TXB0CNTRL.TXREQ is set
    // check if TXB0CNTRL.TXERR is set
    // check if TXB0CNTRL.MLOA is set
    // check if TXB0CNTRL.ABTF is set
    // check if TXB0CNTRL.TXERR

    // TXBxDn: TRANSMIT BUFFER x DATA BYTE n REGISTER
    // (ADDRESS: 36h-3Dh, 46h-4Dh, 56h-5Dh)

    writeArrayRegisters(TXB0SIDH, (uint8_t*)&buf, 8);

    bitModifyRegister(reg::TXB0CTRL, 0x08, 0x08); // set txreq

}

void MCP25625::transmitFrame(CanFrame &canFrame) {
    ESP_LOGI(TAG, "send message");
    struct alignas(32) buf {
        uint8_t sidh; // sid[10:3]
        uint8_t sidl; // sid[2:0] ide eid[17:16]
        uint8_t eid8; // eid[15:8]
        uint8_t eid0; // eid[7:0]
        uint8_t dlc;    // rtr dlc[3:0]
        uint8_t data[8];
    } buf;

    memset(&buf, 0, sizeof(buf));
    if (canFrame.extended) {
        buf.sidh = canFrame.identifier >> 21;
        buf.sidl = ((canFrame.identifier >> (18 - 5)) & 0xe0) // upper 3 bits hence -5
            | ((canFrame.identifier >> 16) & 0x03);
        buf.eid8 = (canFrame.identifier >> 8) & 0xff;
        buf.eid0 = (canFrame.identifier >> 0) & 0xff;
        buf.sidl |= 0x08; // ide
    } else {
        buf.sidh = canFrame.identifier >> 3;
        buf.sidl = ((canFrame.identifier & 0x07) << 5);
    }
    buf.dlc = canFrame.length; // todo rtr
    memcpy(buf.data, canFrame.data, canFrame.length);

    writeArrayRegisters(TXB0SIDH, (uint8_t*)&buf, 8);
    // frame is repeated until TXREQ is cleared
    bitModifyRegister(reg::CANCTRL, 0x08, 0x08); // one shot mode
    bitModifyRegister(reg::TXB0CTRL, 0x08, 0x08); // set txreq
}

void MCP25625::setFilter() {
// RXF filter value
// RXM filter mask
    uint16_t rxm = 0x07ff;
    uint16_t sid = 0x60e;

    writeRegister(reg::RXF0SIDH, (sid >> 3));
    writeRegister(reg::RXF0SIDL, (sid & 0x07) << 5);
    writeRegister(reg::RXM0SIDH, (rxm >> 3));
    writeRegister(reg::RXM0SIDL, (rxm & 0x07) << 5);

}

void MCP25625::testReceiveStatus() {
    uint8_t canintf;
    readRegister(reg::CANINTF, canintf);
    uint8_t caninte;
    readRegister(reg::CANINTE, caninte);
    uint8_t eflg;
    readRegister(reg::EFLG, eflg);
    uint8_t rec;
    readRegister(reg::REC, rec);
    ESP_LOGI(TAG, "receive status canintf: %x caninte: %x eflg %x rec %d", 
        canintf, caninte, eflg, rec);
}

void MCP25625::startReceiveMessages() {
    ESP_LOGI(TAG, "start receive messages");
    reset();
    timing();
    bitModifyRegister(reg::RXB0CTRL, 0x60, 0x60); // receive any message
    bitModifyRegister(reg::CANINTE, 0x01, 0x01); // RX0IE todo abstract
    REQOP normalMode(0); // get out of configuration mode
    bitModifyRegister(reg::CANCTRL, normalMode);
    bitModifyRegister(reg::EFLG, 0x40, 0x00);
    bitModifyRegister(reg::CANINTF, 0x20, 0x00);
}

// defer reading the message from the receive buffer
// CANSTAT:ICOD to demultiplex interrupt
void IRAM_ATTR MCP25625::receiveInterruptISR(void *arg) {
    timestamp_t timestamp = esp_log_timestamp();
    BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
    BaseType_t err = xQueueSendFromISR(*gg, &timestamp, &xHigherPriorityTaskWokenByPost);
    // ESP_DRAM_LOGI(TAG, "isr %d", pdTRUE);
    if (xHigherPriorityTaskWokenByPost) {
        portYIELD_FROM_ISR();
    }
}

// quickly read the message from the receive buffer and defer it to the app
// this must take less than a CAN message time to prevent buffer overflow
void MCP25625::receiveMessageTask(void * pvParameters) {
    ESP_LOGI(TAG, "starting can receive task");

    MCP25625 * self = static_cast<MCP25625*>(pvParameters);
    while (true) {
        timestamp_t timestamp;
        if (xQueueReceive(self->receiveISRQueue, &timestamp, portMAX_DELAY)) {
            receive_msg_t message;
            bool success = self->receiveMessage(&message);
            if (success) {
                xQueueSend(self->receiveMessageQueue, &message, portMAX_DELAY);
            } else {
                ESP_LOGI(TAG, "nothing to receive");
            }
        }
    }

}

// in extended mode, the SID is the most significant 11 bits 
// and the EID is the least significant 18 bits
static uint32_t getIdentifier(uint8_t sidh, uint8_t sidl, uint8_t eid8, uint8_t eid0) {
    const bool ide = IDE::of(sidl);
    return ide
        ? 
            ((sidh & 0xff) << 21) | 
            ((sidl & 0xe0) << (18 - 5)) |
            ((sidl & 0x03) << 16) |
            ((eid8 & 0xff) << 8) |
            ((eid0 & 0xff) << 0)
        : 
            ((sidh & 0xff) << 3) |
            ((sidl & 0xe0) >> 5);
        
    // sid[10:3] SIDH
    // SID[2:0] SIDH
    // EID[17:16] SIDL
    // EID[15:8] EID8
    // EID[7:0] EID0
    // 8+3+2+8+8 = 29 bits
}
