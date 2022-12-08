#pragma once

#include "driver/spi_master.h"

class MCP25625 { // : public canbus
public:
    MCP25625();
    virtual ~MCP25625();

    void init();
    void registerTest(spi_device_handle_t spi);
    void readRegister(uint8_t const address, uint8_t & value);
    void writeRegister(uint8_t const address, uint8_t const value);
private:
    // this is the device object
    spi_device_handle_t spi;

    // the spi interface requires int16_t, but mcp uses only uint8_t
    enum cmd {
        READ = 0x03,
        WRITE = 0x02,
    };
};
