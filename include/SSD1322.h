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
    // Constructor with pin configuration
    SSD1322(int OLED_CS_PIN, int OLED_DC_PIN, int OLED_HEIGHT_SIZE, int OLED_WIDTH_SIZE, int OLED_RESET_PIN);

    // Initialize display with current configuration
    void SSD1322_INIT();

    // individual instances
    SSD1322_HW_DRIVER driver;
    SSD1322_API api;
    SSD1322_GFX gfx;

private:
    SSD1322_CONFIG config;
};

#endif /* SSD1322_H */