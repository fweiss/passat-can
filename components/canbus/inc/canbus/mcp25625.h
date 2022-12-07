#pragma once

#include "driver/spi_master.h"

class MCP25625 { // : public canbus
public:
    MCP25625();
    virtual ~MCP25625();

    void init();
    void registerTest(spi_device_handle_t spi);
};
