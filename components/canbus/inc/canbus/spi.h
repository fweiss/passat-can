#pragma once

#include <stdint.h>
#include "driver/spi_master.h"
#include "freertos/semphr.h"

// mcp25625 spi-ness
// specific commands

struct FieldValue;

class SPI {
public:
    void init();
    void deinit();

    void readRegister(uint8_t const address, uint8_t & value);
    void readArrayRegisters(uint8_t startAddress, uint8_t * data, uint8_t count);
    void readBufferRegisters(uint8_t rxAddressIndex, uint8_t * data, uint8_t count);
    void writeRegister(uint8_t const address, uint8_t const value);
    void writeArrayRegisters(uint8_t startAddress, uint8_t * data, uint8_t count);

    void bitModifyRegister(uint8_t const address, uint8_t const mask, uint8_t value);
    void bitModifyRegister(uint8_t const address, FieldValue f);
    void reset();
private:
    const spi_host_device_t canHost = SPI3_HOST;
    // this is the device object
    spi_device_handle_t spi;

    // needed to allow tasks to use spi_device_transmit
    SemaphoreHandle_t transactionMutex;
    const TickType_t transactionMutexBlockTime = 50 /portTICK_PERIOD_MS;

    // the spi interface requires int16_t, but mcp uses only uint8_t
    // mc25625 SPI command enumeration
    enum cmd {
        WRITE = 0x02,
        READ = 0x03,
        BIT_MODIFY = 0x05,
        LOAD_TX_BUFFER = 0x40,
        RTS = 0x80,
        READ_RX_BUFFER = 0x90,
        READ_STATUS = 0xa0,
        RX_STATUS = 0xb0,
        RESET = 0xc0,
    };
    // interrupt codes in CANSTAT
    enum icod {
        ICOD_MERR = 0,
        ICOD_ERR = 1,
        ICOD_WAK = 2,
        ICOD_TX0 = 3,
        ICOD_TX1 = 4,
        ICOD_TX2 = 5,
        ICOD_RX0 = 6,
        ICOD_RXB = 7,
    };
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
