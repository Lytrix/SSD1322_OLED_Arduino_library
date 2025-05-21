#include <Arduino.h>
#include <SPI.h>
#include "ssd1322.h"
#include "dma_test.h"

#define SSD1322_CS   10  // Chip select
#define SSD1322_DC    9  // Data/Command
#define SSD1322_RES   8  // Reset

DMAMEM uint8_t buffer[128] __attribute__((aligned(32)));

MinimalDMASPI dmaSPI;

SSD1322 display(SSD1322_CS, SSD1322_DC, SSD1322_RES);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    Serial.println("Minimal DMA SPI Example");

    SPI.begin();
    dmaSPI.begin();
    display.setDMA(&dmaSPI);       // 3. Link the DMA instance to the display

    display.begin();
    display.fill(0xFF); // All pixels on (4-bit max)
    Serial.println("Starting DMA transfer...");
    display.writeBufferDMA();
    Serial.println("DMA transfer complete, all pixels ON");

    Serial.println("Setup DMA SPI");
    //MinimalDMASPI::dmaObjects[0] = &dmaSPI;
    
    delay(1000);

    Serial.println("DMA SPI setup complete");

    Serial.println("Send via DMA to SPI");
    display.writeBufferDMA();
    delay(1000);
    Serial.println("All pixels ON using DMA");
}

void loop() {

}