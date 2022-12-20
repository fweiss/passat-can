#pragma once

#include "driver/spi_master.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

struct FieldValue;
struct receive_msg_t;

class MCP25625 { // : public canbus
public:
    MCP25625();
    virtual ~MCP25625();

    void init();
    void deinit();
    void readRegister(uint8_t const address, uint8_t & value);
    void writeRegister(uint8_t const address, uint8_t const value);
    void bitModifyRegister(uint8_t const address, uint8_t const mask, uint8_t value);
    void bitModifyRegister(uint8_t const address, FieldValue f);
    void reset();

    void testRegisters();
    void testReceive();
    void testLoopBack();
private:
    // this is the device object
    spi_device_handle_t spi;
    intr_handle_t receiveInterruptHandle;
    QueueHandle_t receiveQueue;
    const spi_host_device_t canHost = SPI3_HOST;

    // the spi interface requires int16_t, but mcp uses only uint8_t
    // mc25625 SPI command enumeration
    enum cmd {
        READ = 0x03,
        WRITE = 0x02,
        BIT_MODIFY = 0x05,
        RESET = 0xc0,
    };
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
    void attachReceiveInterrupt();
    void detachReceiveInterrupt();
    static void receiveInterruptISR(void *arg);
    void receiveDataEnqueue();
};

const size_t max = 8;
struct receive_msg_t {
    uint32_t identifier;
    uint8_t data_length_code;
    uint8_t data[max];
};

// some template magic to simplify writing bit fields
// for use with bitModifyRegister
struct FieldValue {
    const uint8_t mask;
    const uint8_t bits;
    FieldValue(uint8_t mask, uint8_t bits) : mask(mask), bits(bits) {}
};
// WID is the number of bits in the field
// LSB is the least significant bit offset from the right
template<uint8_t WID, uint8_t LSB>
struct Field : public FieldValue {
    Field(uint8_t value) : FieldValue((((1 << WID) - 1) << LSB), value << LSB) { }
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
