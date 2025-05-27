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
    SSD1322(const SSD1322_CONFIG& config_in = SSD1322_CONFIG());

    // individual instances
    SSD1322_HW_DRIVER driver;
    SSD1322_API api;
    SSD1322_GFX gfx;
    SSD1322_CONFIG config;
    //SSD1322_DMA dmaSpi;

    void begin();
    void setDMASPI();

#ifdef __IMXRT1062__
// Forward declare SSD1322_DMA class
    void sendFrameBufferDMA(uint8_t* frame_buffer, size_t size = FRAMEBUFFER_SIZE);
private:
    uint8_t* dmaBuffer = nullptr;
    bool useDMA = false;
#endif
};
#endif /* SSD1322_H */