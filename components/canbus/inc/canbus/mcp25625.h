#pragma once

#include "driver/spi_master.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "spi.h"

struct receive_msg_t;

class MCP25625 : public SPI {
public:
    QueueHandle_t receiveQueue;
    MCP25625();
    virtual ~MCP25625();

    void init();
    void deinit();

    void attachReceiveInterrupt();
    void detachReceiveInterrupt();
    void startReceiveMessages();
    bool receiveMessage(receive_msg_t & message);

    void testRegisters();
    void testReceive();
    void testLoopBack();
    void testReceiveStatus();
private:

    intr_handle_t receiveInterruptHandle;

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
        REC = 0x1d,
        EFLG = 0x2d,
        TXB0SIDH = 0x31,
        TXB0SIDL = 0x32,
        TXB0DLC = 0x35,
        TXB0CTRL = 0x30,
        RXB0D0 = 0x66,
        RXB0SIDH = 0x61,
        RXB0SIDL = 0x62,
    };
    void timing();
    static void receiveInterruptISR(void *arg);
};

const size_t max = 8;
struct receive_msg_t {
    uint32_t identifier;
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
