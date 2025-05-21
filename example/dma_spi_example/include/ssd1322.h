#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "dma_test.h"

class SSD1322 {
public:
    SSD1322(uint8_t cs, uint8_t dc, uint8_t res);
    void begin();
    void fill(uint8_t value);
    void writeBuffer();
    void writeBufferDMA();
    void clear();
    uint8_t* getBuffer();
    static constexpr size_t BufferSize = 8192;

private:
    void command(uint8_t cmd);
    void data(uint8_t data);
    void hardwareReset();
    void initSequence();
    uint8_t _cs, _dc, _res;
    uint8_t _buffer[BufferSize];
}; 