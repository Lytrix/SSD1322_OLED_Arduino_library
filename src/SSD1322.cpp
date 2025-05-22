#include "SSD1322.h"
#include "SSD1322_Config.h"
#ifdef __IMXRT1062__
#endif


//====================== Constructor ========================//
SSD1322::SSD1322(const SSD1322_CONFIG& config_in)
    : driver(config_in.OLED_CS_PIN, config_in.OLED_DC_PIN, config_in.OLED_RESET_PIN),
      api(&driver),
      gfx(&api, config_in.OLED_HEIGHT, config_in.OLED_WIDTH),
      config(config_in),
    #ifdef __IMXRT1062__
      dmaSpi(SPI, 0) // or SPI1, 1 for other SPI ports
    #endif
    
{
    // Ensure pins are properly set up
    pinMode(config.OLED_CS_PIN, OUTPUT);
    pinMode(config.OLED_DC_PIN, OUTPUT);
    if (config.OLED_RESET_PIN != -1) {
        pinMode(config.OLED_RESET_PIN, OUTPUT);
    }
}

void SSD1322::begin() {
    // Pin setup (repeat for safety)
    pinMode(config.OLED_CS_PIN, OUTPUT);
    pinMode(config.OLED_DC_PIN, OUTPUT);
    if (config.OLED_RESET_PIN != -1) {
        pinMode(config.OLED_RESET_PIN, OUTPUT);
    }

#ifdef __IMXRT1062__
    // DMA SPI and buffer setup
    if (!dmaBuffer) {
        static DMAMEM uint8_t static_dma_buffer[FRAMEBUFFER_SIZE] __attribute__((aligned(32)));
        dmaBuffer = static_dma_buffer;
        // Share the DMA buffer with the API
        api.setDMABuffer(dmaBuffer);
    }
    Serial.println("SSD1322: DMA SPI and buffer setup");
    dmaSpi.begin(config.OLED_CS_PIN, SPISettings(config.SPI_CLOCK, MSBFIRST, SPI_MODE0));
    useDMA = true;
#else
    Serial.println("SSD1322: No DMA SPI");
    SPI.begin();
    delay(100); // Longer delay as in example
#endif  
    Serial.println("SSD1322: DMA SPI and buffer setup done");

    // Display initialization
    Serial.println("SSD1322: Display initialization");
    api.SSD1322_API_init();
    Serial.println("SSD1322: Display initialization done");
}

#ifdef __IMXRT1062__
void SSD1322::sendFrameBufferDMA(uint8_t* frame_buffer, size_t size) {
    if (!dmaBuffer || !useDMA) {
        Serial.println("SSD1322: DMA not initialized or disabled");
        return;
    }
    
    // Use the API's send_buffer_DMA method
    api.SSD1322_API_set_window(0, 63, 0, 63); // Full window, adjust as needed
    api.SSD1322_API_send_buffer_DMA(frame_buffer, size, dmaBuffer);
}
#endif

// SSD1322_DMA myDmaSpi(SPI, 0);
// void SSD1322::setDMASPI() {
//     api.setDMASPI(&myDmaSpi);
// }