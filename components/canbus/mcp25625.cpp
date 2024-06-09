#include "mcp25625.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <string.h>

static char const * const TAG = "mcp25625";

// forward
static uint32_t getIdentifier(uint8_t sidh, uint8_t sidl, uint8_t eid8, uint8_t eid0);

MCP25625::MCP25625() {
    // debug will increase ISR latency
    esp_log_level_set(TAG, ESP_LOG_INFO);

    receiveMessageQueue = xQueueCreate(20, sizeof(receive_msg_t));
    transmitFrameQueue = xQueueCreate(20, sizeof(CanStatus));
    errorQueue = xQueueCreate(20, sizeof(CanStatus));

    interruptSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(interruptTask, "interrupt task", 4048, this, 5, NULL, APP_CPU_NUM);
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

// todo not just receive, but also transmit
void MCP25625::attachInterrupt() {
    esp_err_t err;

    ESP_LOGI(TAG, "attach receive interrupt");

    const gpio_num_t interruptPin = GPIO_NUM_21; // fixme extract to configuration
    esp_rom_gpio_pad_select_gpio(interruptPin);
    gpio_set_direction(interruptPin, GPIO_MODE_INPUT);
    gpio_pulldown_dis(interruptPin);
    gpio_pullup_en(interruptPin);
    gpio_set_intr_type(interruptPin, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    err = gpio_isr_handler_add(interruptPin, interruptISR, this);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "give this %p", this);
    } else {
        ESP_LOGE(TAG, "isr handler add err: %x", err);
    }

    // enable interrupts
    bitModifyRegister(reg::CANINTE, 0xa5, 0xa5); // MERRIE, ERRIE, RX0IE, TX0IE
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

// todo parameterize RXn, use special SPI command
void MCP25625::readFrameBuffer(FrameBuffer &frameBuffer) {
    // readArrayRegisters(RXB0SIDH, (uint8_t*)&frameBuffer, sizeof(frameBuffer));
    // todo parameterize the receive buffer index
    uint8_t rxBufferIndex = 0;
    readBufferRegisters(rxBufferIndex, (uint8_t*)&frameBuffer, sizeof(frameBuffer));
}
// todo parameterize TXn, use special SPI command
void MCP25625::writeFrameBuffer(FrameBuffer &frameBuffer) {
    writeArrayRegisters(TXB0SIDH, (uint8_t*)&frameBuffer, sizeof(frameBuffer));
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
    // bitModifyRegister(reg::CANINTE, 0x54, 0x54); // TX0IE

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
    ESP_LOGD(TAG, "send message");
    struct alignas(32) buf {
        uint8_t sidh; // sid[10:3]
        uint8_t sidl; // sid[2:0] ide eid[17:16]
        uint8_t eid8; // eid[15:8]
        uint8_t eid0; // eid[7:0]
        uint8_t dlc;
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
    buf.dlc = canFrame.length | (canFrame.remote ? 0x40 : 0);
    memcpy(buf.data, canFrame.data, canFrame.length);

    writeArrayRegisters(TXB0SIDH, (uint8_t*)&buf, canFrame.length + 5);

    // bitModifyRegister(reg::CANINTE, 0x80, 0x80); // MERRE
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

void MCP25625::getStatus(CanStatus &canStatus) {
    readRegister(reg::CANINTF, canStatus.canintf);
    readRegister(reg::CANINTE, canStatus.caninte);
    readRegister(reg::TXB0CTRL, canStatus.tb0ctrl);
    readRegister(reg::EFLG, canStatus.eflg);
    readRegister(reg::TEC, canStatus.tec);
}

void MCP25625::startReceiveMessages() {
    ESP_LOGI(TAG, "start receive messages");
    reset();
    timing();
    bitModifyRegister(reg::RXB0CTRL, 0x60, 0x60); // receive any message
    // bitModifyRegister(reg::CANINTE, 0x01, 0x01); // RX0IE todo abstract
    // bitModifyRegister(reg::CANINTE, 0x04, 0x04); // TX0IE todo abstract
    bitModifyRegister(reg::CANINTE, 0xa5, 0xa5); 
    REQOP normalMode(0); // get out of configuration mode
    bitModifyRegister(reg::CANCTRL, normalMode);
    bitModifyRegister(reg::EFLG, 0x40, 0x00);
    bitModifyRegister(reg::CANINTF, 0x20, 0x00);
}

// MCP25625 interrupt service routines
// interruptISR -> interruptSemaphore -> interruptTask:
// rx: receiveISRQueue(timestamp) -> receiveMessageTask -> receiveMessageQueue(message) -> app

// bridge ISR to task
SemaphoreHandle_t MCP25625::interruptSemaphore = nullptr;

// interrupt handler registered for the INT GPIO pin
void IRAM_ATTR MCP25625::interruptISR(void *arg) {
    MCP25625 * self = static_cast<MCP25625*>(arg);
    BaseType_t higherProrityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(self->interruptSemaphore, &higherProrityTaskWoken);
    if (higherProrityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// task that dispatches interrupt events
// MERRF: Message Error Interrupt Flag bit
// ERRIF: Error Interrupt Flag bit (multiple sources in the EFLG register)
// EFLG: (RX1OVR, RX0OVR, TXBO, TXEP, RXEP, TXWAR, RXWAR)
//
// need to "re-enter" to check if other flags are set
// loop and wait on semaphore from ISR
// loop and read registers to check for interrupt source
// dispatch handler based on interrupt source
// or exit loop and wait for next interrupt
void MCP25625::interruptTask(void * pvParameters) {
    MCP25625 * self = static_cast<MCP25625*>(pvParameters);

    BaseType_t status;
    while (true) {
        // block and wait for an interrupt
        // todo no timeout, but check status
        status = xQueueSemaphoreTake(self->interruptSemaphore, portMAX_DELAY);

        struct alignas(32) {
            uint8_t caninte;
            uint8_t canintf;
            uint8_t eflg;
            uint8_t canstat;
        } interruptFlagsBuffer;

        while (true) { // check for mutliple flags
            self->readArrayRegisters(reg::CANINTE, (uint8_t*)&interruptFlagsBuffer, sizeof(interruptFlagsBuffer));
            uint8_t icod = (interruptFlagsBuffer.canstat >> 1) & 0x07;
            // if ((interruptFlagsBuffer.canintf & interruptFlagsBuffer.caninte) == 0) {
            if ((interruptFlagsBuffer.canintf) == 0) {
                break;
            }

        // const timestamp_t timestamp = esp_log_timestamp();
        // const uint8_t icod = getIcod();
        const uint16_t ticksToWait = 1000;
        CanStatus canStatus;
        ESP_LOGD(TAG, "interrupt %d", icod);
        switch (icod) { // [Register 4.35]
            case 0: // 000 = MERR interrupt
                self->getStatus(canStatus);
                canStatus.icod = icod + 0x80;
                ESP_LOGD(TAG, "MERR interrupt canintf: %x caninte: %x eflg %x rec %d", 
                    canStatus.canintf, canStatus.caninte, canStatus.eflg, canStatus.tec);

                // canStatus = self->getStatus();
                xQueueSend(self->errorQueue, &canStatus, ticksToWait);

                self->bitModifyRegister(reg::CANINTF, 0x80, 0x00); // MERRF
                break;
            case 0x01: // 001 = ERR interrupt
                // EFLG indicates: RX1OVR, RX0OVR, TXBO, TXEP, RXEP, TXWAR, RXWAR, EWARN
                self->getStatus(canStatus);
                canStatus.icod = icod;
                ESP_LOGD(TAG, "ERR interrupt canintf: %x caninte: %x eflg %x rec %d", 
                    canStatus.canintf, canStatus.caninte, canStatus.eflg, canStatus.tec);

                // canStatus = self->getStatus();
                xQueueSend(self->errorQueue, &canStatus, ticksToWait);

                // self->bitModifyRegister(reg::CANINTF, 0x20, 0x00); // ERRIF
                // self->bitModifyRegister(reg::EFLG, 0x40, 0x00); // WAKIF
                self->bitModifyRegister(reg::EFLG, 0xC0, 0x00); // RX1OVR, RX0OVR
                self->bitModifyRegister(reg::CANINTF, 0x20, 0x00); // ERRIF
                break;
            case 0x03: // 011 = TXB0 interrupt
            case 0x04: // 100 = TXB1 interrupt
                ESP_LOGD(TAG, "TXB0 interrupt");

                self->getStatus(canStatus);
                canStatus.icod = icod;

                xQueueSend(self->transmitFrameQueue, &canStatus, ticksToWait);

                self->bitModifyRegister(reg::CANINTF, 0x04, 0x00); // clear tx0if
                // self->bitModifyRegister(reg::CANINTF, 0x80, 0x00); // MERRF
                break;
            case 0x06: // 110 = RXB0 interrupt
            {
                ESP_LOGD(TAG, "RXB0 interrupt");
                FrameBuffer frameBuffer;
                self->readFrameBuffer(frameBuffer);
                // self->bitModifyRegister(reg::CANINTF, 0x01, 0x00); // clear rx0if

                // see receiveMessage()
                receive_msg_t message;
                FrameBuffer & buf = frameBuffer;
                message.identifier = getIdentifier(buf.sidh, buf.sidl, buf.eid8, buf.eid0);
                message.flags.srr = SRR::of(buf.sidl);
                message.flags.ide = IDE::of(buf.sidl);
                message.data_length_code = buf.dlc & 0x0f;
                memcpy(message.data, buf.data, sizeof(buf.data));

                xQueueSend(self->receiveMessageQueue, &message, ticksToWait);
                // xQueueSend(self->receiveISRQueue, &timestamp, ticksToWait);
                break;
            }
            default:
                ESP_LOGE(TAG, "unhandled interrupt icod %d", icod);
                break;
        }
        }
        // self->readRegister(reg::CANINTF, canStatus.canintf);
        // if (canStatus.canintf != 0) {
        //     ESP_LOGE(TAG, "not cleared canintf %x icod %x", canStatus.canintf, icod);
        // }
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
