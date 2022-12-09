#pragma once

#include "driver/spi_master.h"

class MCP25625 { // : public canbus
public:
    MCP25625();
    virtual ~MCP25625();

    void init();
    void readRegister(uint8_t const address, uint8_t & value);
    void writeRegister(uint8_t const address, uint8_t const value);
    void bitModifyRegister(uint8_t const address, uint8_t const mask, uint8_t value);
    void reset();

    void registerTest();
    void receiveTest();
private:
    // this is the device object
    spi_device_handle_t spi;

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
        CANCTRL = 0x0e,
        CANSTAT = 0x0f,
        CANINTF = 0x2c,
        CNF1 = 0x2a,
        CNF2 = 0x29,
        CNF3 = 0x28,
        RXB0DLC = 0x65,
        RXB0CTRL = 0x60,
    };
    void timing();
};
