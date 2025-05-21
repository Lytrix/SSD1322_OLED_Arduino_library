#include "ssd1322.h"
#include "dma_test.h"

SSD1322::SSD1322(uint8_t cs, uint8_t dc, uint8_t res)
    : _cs(cs), _dc(dc), _res(res) {
    memset(_buffer, 0, BufferSize);
}

void SSD1322::begin() {
    pinMode(_cs, OUTPUT);
    pinMode(_dc, OUTPUT);
    pinMode(_res, OUTPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_dc, HIGH);
    hardwareReset();
    initSequence();
}

void SSD1322::hardwareReset() {
    digitalWrite(_res, LOW);
    delay(10);
    digitalWrite(_res, HIGH);
    delay(10);
}

void SSD1322::initSequence() {
    command(0xFD); data(0x12); // Unlock
    command(0xAE); // Display off
    command(0xB3); data(0x91); // Display clock divide
    command(0xCA); data(0x3F); // Multiplex ratio
    command(0xA2); data(0x00); // Display offset
    command(0xA1); data(0x00); // Start line
    command(0xA0); data(0x14); data(0x11); // Remap
    command(0xAB); data(0x01); // Enable internal VDD regulator
    command(0xB4); data(0xA0); data(0xFD); // Set segment low voltage
    command(0xC1); data(0x9F); // Set contrast
    command(0xC7); data(0x0F); // Master contrast
    command(0xB9); // Use default linear grayscale table
    command(0xB1); data(0xE2); // Set phase length
    command(0xD1); data(0x82); data(0x20); // Display enhancement
    command(0xBB); data(0x1F); // Precharge voltage
    command(0xB6); data(0x08); // Precharge period
    command(0xBE); data(0x07); // VCOMH
    command(0xA6); // Normal display
    command(0xAF); // Display on
}

void SSD1322::command(uint8_t cmd) {
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    SPI.transfer(cmd);
    digitalWrite(_cs, HIGH);
}

void SSD1322::data(uint8_t data) {
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    SPI.transfer(data);
    digitalWrite(_cs, HIGH);
}

void SSD1322::fill(uint8_t value) {
    memset(_buffer, value, BufferSize);
}

uint8_t* SSD1322::getBuffer() {
    return _buffer;
}

void SSD1322::writeBuffer() {
    // Set column and row address (see datasheet for your display)
    command(0x15); data(0x1C); data(0x5B); // Set column address (28-91)
    command(0x75); data(0x00); data(0x3F); // Set row address (0-63)
    command(0x5C); // Write RAM

    digitalWrite(_dc, HIGH); // Data mode
    digitalWrite(_cs, LOW);
    SPI.transfer(_buffer, BufferSize);
    digitalWrite(_cs, HIGH);
}

void SSD1322::writeBufferDMA() {
    static LowLevelDMATransfer dma;
    // Set column and row address
    command(0x15); data(0x1C); data(0x5B);
    command(0x75); data(0x00); data(0x3F);
    command(0x5C);

    digitalWrite(_dc, HIGH); // Data mode
    digitalWrite(_cs, LOW);

    // Copy display buffer to DMA source buffer
    LowLevelDMATransfer::setSource(_buffer, BufferSize);
    // Set destination to SPI transmit register (LPSPI4_TDR)
    // This is already set in the LowLevelDMATransfer::begin() for _dest, but for SPI, you would set:
    // DMA_TCD_DADDR(DMA_CHANNEL) = (uint32_t)&IMXRT_LPSPI4_S.TDR;
    // For now, we use the default dest buffer as in the example
    dma.begin();

    // Wait for DMA to complete (toggle LED in ISR for now)
    delay(10); // Placeholder for actual completion check
    dma.enable();
    digitalWrite(_cs, HIGH);
}

void SSD1322::clear() {
    fill(0x00);
    writeBuffer();
} 