#pragma once

#include "driver/spi_master.h"

class MCP25625 { // : public canbus
public:
    MCP25625();
    virtual ~MCP25625();

    void init();
    void readRegister(uint8_t const address, uint8_t & value);
    void writeRegister(uint8_t const address, uint8_t const value);

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
    };
    // mcp25625 register address enumeration
    enum reg {
        CANCTRL = 0x0e,
        CNF1 = 0x2a,
    };
};
