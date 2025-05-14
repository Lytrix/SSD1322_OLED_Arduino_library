#ifndef SSD1322_H
#define SSD1322_H

#include "SSD1322_API.h"
#include "SSD1322_GFX.h"
#include "SSD1322_HW_Driver.h"
#include "SSD1322_Config.h"

// wrapper class of individual classes
class SSD1322
{
public:
    SSD1322(int OLED_CS_PIN, int OLED_DC_PIN, int OLED_HEIGHT_SIZE, int OLED_WIDTH_SIZE, int OLED_RESET_PIN = -1);

    // individual instances
    SSD1322_HW_DRIVER driver;
    SSD1322_API api;
    SSD1322_GFX gfx;

#ifdef __IMXRT1062__     // only use on Teensy 4.x
    // Step one chunk of an in-progress DMA update
    // returns true when that frame is fully pushed
    bool drawFrameBufferInterruptible(uint8_t *frame_buffer);
    
    // Initialize DMA circular buffer
    void beginDMA();
    
    // Stop DMA operations and return to normal mode
    void endDMA();
    
    // Check if DMA is active
    bool isDMARunning();
#endif
};

#endif /* SSD1322_H */