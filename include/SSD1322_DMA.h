#ifndef SSD1322_DMA_H
#define SSD1322_DMA_H

#include <Arduino.h>
#include <SPI.h>
#include <DMAChannel.h>
#include "SSD1322_Config.h"

#ifdef __IMXRT1062__  // Only compile for Teensy 4.x

// Forward declaration of SSD1322 class
class SSD1322;

class SSD1322_DMA {
public:
    // Default constructor
    SSD1322_DMA();
    
    // Initialize with display reference
    void init(SSD1322& display);

    // Start a continuous 3-segment circular DMA update
    void begin();

    // Stop DMA and restore normal SPI operation
    void stop();

    // Returns true if DMA is currently running
    bool isRunning() const;
    
    // Process one chunk of an interruptible buffer update
    // Returns true when the frame is completely sent
    bool draw_framebuffer_interruptible(uint8_t *src_buffer);

private:
    SSD1322*       _disp;
    DMAChannel     _dma;
    DMASetting     _settings[SEGMENTS];

    // State flags
    volatile bool    _dmaComplete;
    volatile uint32_t _dmaTriggerCount;
    volatile bool    _continuousMode;
    volatile uint32_t _pendingRxCount;
    volatile uint32_t _dmaFifoResets;
    volatile uint32_t _dmaRxErrors;
    
    // Buffer processing helpers
    void pack_pixels(uint8_t *src_buffer, uint8_t *packed_buffer, int size);
    void setup_display_window();

    // Internal helpers for orchestrated circular DMA
    void setupDmamuxChannel();
    void configureDmaChain();
    void configureSpiFifos();
    void primeTxFifoAndStartDma();

    void waitTransmitComplete();
    void dmaCircularCompleteISR();

    // ISR friend so it can update private flags
    friend void dma_circular_complete_isr();
};

// Global instance accessible to the SSD1322 class
extern SSD1322_DMA dmaDriver;

#endif // __IMXRT1062__

#endif // SSD1322_DMA_H