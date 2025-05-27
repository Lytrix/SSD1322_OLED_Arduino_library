#ifndef SSD1322_CONFIG_H
#define SSD1322_CONFIG_H

#include <Arduino.h>

#define DEBUG false

// Display Configuration
struct SSD1322_CONFIG {
    // Pin Configuration
    int OLED_CS_PIN = 10;
    int OLED_DC_PIN = 9;
    int OLED_RESET_PIN = 8;
    int OLED_CLOCK_PIN = 13;
    int OLED_DATA_PIN = 11;  // MOSI
    
    // Display Dimensions
    int OLED_WIDTH = 256;
    int OLED_HEIGHT = 64;
    
    // SPI Configuration
    uint32_t SPI_CLOCK = 8000000;  // 8MHz default
    
    // Display Settings
    uint8_t CONTRAST = 255;
    
    // Buffer Settings
    int BUFFER_WIDTH = 256;
    int BUFFER_HEIGHT = 64;
};

// Display dimensions
#define DISPLAY_WIDTH   256
#define DISPLAY_HEIGHT  64

// Frame buffer size in bytes (4 bits per pixel = 2 pixels per byte)
#define FRAMEBUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)

// Display controller command codes
#define SSD1322_SET_COLUMN    0x15
#define SSD1322_SET_ROW       0x75
#define SSD1322_WRITE_RAM     0x5C
#define SSD1322_COLUMN_START  0x00
#define SSD1322_COLUMN_END    0x77  // For 256 pixel width (0x3F for 128px)
#define SSD1322_ROW_START     0x00
#define SSD1322_ROW_END       0x3F  // For 64 pixel height

#endif /* SSD1322_CONFIG_H */ 