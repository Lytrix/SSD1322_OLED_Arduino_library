#include "SSD1322.h"

//====================== Constructor ========================//
SSD1322::SSD1322(int OLED_CS_PIN, int OLED_DC_PIN, int OLED_HEIGHT_SIZE, int OLED_WIDTH_SIZE, int OLED_RESET_PIN) : 
    driver(OLED_CS_PIN, OLED_DC_PIN, OLED_RESET_PIN, 8000000), // 8MHz SPI clock
    api(&driver), 
    gfx(&api, OLED_HEIGHT_SIZE, OLED_WIDTH_SIZE) {
    
    // Ensure pins are properly set up
    pinMode(OLED_CS_PIN, OUTPUT);
    pinMode(OLED_DC_PIN, OUTPUT);
    if (OLED_RESET_PIN != -1) {
        pinMode(OLED_RESET_PIN, OUTPUT);
    }
}