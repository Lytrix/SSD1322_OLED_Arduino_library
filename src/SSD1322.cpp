#include "SSD1322.h"
#include "SSD1322_Config.h"
#include "SSD1322_DMA.h"

//====================== Constructor ========================//
SSD1322::SSD1322(int OLED_CS_PIN, int OLED_DC_PIN, int OLED_HEIGHT_SIZE, int OLED_WIDTH_SIZE, int OLED_RESET_PIN) : 
    driver(OLED_CS_PIN, OLED_DC_PIN, OLED_RESET_PIN), // default SPI clock
    api(&driver), 
    gfx(&api, OLED_HEIGHT_SIZE, OLED_WIDTH_SIZE) {
    
    // Ensure pins are properly set up
    pinMode(OLED_CS_PIN, OUTPUT);
    pinMode(OLED_DC_PIN, OUTPUT);
    if (OLED_RESET_PIN != -1) {
        pinMode(OLED_RESET_PIN, OUTPUT);
    }
}

#ifdef __IMXRT1062__
// Initialize the DMA driver
void SSD1322::beginDMA() {
    // Initialize the DMA driver with this display instance
    dmaDriver.init(*this);
    
    // Start the DMA circular buffer
    dmaDriver.begin();
}

// Stop DMA operations
void SSD1322::endDMA() {
    dmaDriver.stop();
}

// Check if DMA is active
bool SSD1322::isDMARunning() {
    return dmaDriver.isRunning();
}

// Step one chunk of an in-progress DMA update; returns true when frame fully pushed
bool SSD1322::drawFrameBufferInterruptible(uint8_t *frame_buffer) {
    return dmaDriver.draw_framebuffer_interruptible(frame_buffer);
}
#endif