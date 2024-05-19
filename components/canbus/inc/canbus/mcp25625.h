#pragma once

#include "driver/spi_master.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "spi.h"

struct receive_msg_t;
typedef uint32_t timestamp_t;

struct CanFrame {
    uint32_t identifier;
    uint8_t length;
    uint8_t data[8];
    bool extended;
    bool remote;
};

struct CanStatus {
    uint8_t eflg;
    uint8_t tec;
    uint8_t canintf;
    uint8_t caninte;
    uint8_t tb0ctrl;
};

class MCP25625 : public SPI {
public:
    MCP25625();
    virtual ~MCP25625();

    QueueHandle_t receiveISRQueue;
    QueueHandle_t receiveMessageQueue;

    void init();
    void deinit();

    void attachReceiveInterrupt();
    void detachReceiveInterrupt();
    void startReceiveMessages();
    bool receiveMessage(receive_msg_t * message);
    void setFilter();
    
    void sendMessage(uint8_t * payload, size_t len); //deprecated
    void transmitFrame(CanFrame &canFrame);


    void testRegisters();
    void testReceive();
    void testLoopBack();
    void testReceiveStatus();
    CanStatus getStatus();
private:

    intr_handle_t receiveInterruptHandle;
    static void receiveMessageTask(void * pvParameters);

    // mcp25625 register address enumeration
    enum reg {
        CANCTRL = 0x0f,
        CANSTAT = 0x0e,
        CANINTE = 0x2b,
        CANINTF = 0x2c,
        CNF1 = 0x2a,
        CNF2 = 0x29,
        CNF3 = 0x28,
        RXB0DLC = 0x65,
        RXB0CTRL = 0x60,
        TEC = 0x1c,
        REC = 0x1d,
        EFLG = 0x2d,
        TXB0SIDH = 0x31,
        TXB0SIDL = 0x32,
        TXB0DLC = 0x35,
        TXB0CTRL = 0x30,
        RXB0D0 = 0x66,
        RXB0SIDH = 0x61,
        RXB0SIDL = 0x62,
        RXF0SIDH = 0x00,
        RXF0SIDL = 0x01,
        RXM0SIDH = 0x20,
        RXM0SIDL = 0x21,
    };
    void timing();
    static void receiveInterruptISR(void *arg);
    void clearRX0OVR();
};

const size_t max = 8;
struct receive_msg_t {
    uint32_t identifier;
    struct {
        uint8_t srr : 1;
        uint8_t ide : 1;
    } flags;
    uint8_t data_length_code;
    uint8_t data[max];
};

struct SJW : Field<2,6> { // CNF1
    SJW(uint8_t value) : Field(value) {};
};
struct BRP : Field<6, 0> { // CNF1
    BRP(uint8_t value) : Field(value) {};
};
struct SAM : Field<1, 6> {
    SAM(uint8_t value) : Field(value) {};
};
struct PHSEG1 : Field<3, 3> { // CNF2
    PHSEG1(uint8_t value) : Field(value) {};
};
struct PRSEG : Field<3, 0> { // CNF2
    PRSEG(uint8_t value) : Field(value) {};
};
struct PHSEG2 : Field<3, 0> { // CNF3
    PHSEG2(uint8_t value) : Field(value) {};
};
struct REQOP : Field<3, 5> { // CANCTRL
    REQOP(uint8_t value) : Field(value) {}
};
// Standard Frame Remote Transmit Request bit
struct SRR : Field<1, 4> { // TXB0SIDL
    SRR(uint8_t value) : Field(value) {}
    static uint8_t of(uint8_t reg) {
        return (reg >> 4) & 0x01;
    }
};
// Extended Identifier bit
struct IDE : Field<1, 3> { // TXB0SIDL
    IDE(uint8_t value) : Field(value) {}
    static uint8_t of(uint8_t reg) {
        return (reg >> 3) & 0x01;
    }
};
