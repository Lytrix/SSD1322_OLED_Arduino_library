#ifndef SSD1322_CONFIG_H
#define SSD1322_CONFIG_H

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

#endif /* SSD1322_CONFIG_H */ 