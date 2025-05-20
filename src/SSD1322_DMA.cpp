#include "SSD1322_DMA.h"
#include "SSD1322_Config.h"

#if defined(__IMXRT1062__)

#ifdef __IMXRT1062__
// --- Helper prototypes for orchestrated circular DMA ---
//static void setupSpiHardware();
void setupDmamuxChannel();
void configureDmaChain();
void configureSpiFifos();
void primeTxFifoAndStartDma();
#endif

// Global instance that will be accessed from SSD1322 class
SSD1322_DMA dmaDriver;

// Forward declaration for the ISR callback
void dma_circular_complete_isr();

SSD1322_DMA::SSD1322_DMA()
  : _disp(nullptr), _dmaComplete(true), _dmaTriggerCount(0), 
    _continuousMode(false), _pendingRxCount(0), 
    _dmaFifoResets(0), _dmaRxErrors(0)
{
  // Real work happens in init() and begin()
}

// Initialize with display reference
void SSD1322_DMA::init(SSD1322& display) {
  _disp = &display;
}

void SSD1322_DMA::begin() {
  if (!_disp) {
    Serial.println("ERROR: Display not initialized. Call init() first.");
    return;
  }
  
  setupDmamuxChannel();
  configureDmaChain();
  configureSpiFifos();
  primeTxFifoAndStartDma();
  _dmaComplete = false;
}

void SSD1322_DMA::stop() {
  LPSPI4_DER = 0;
  _dma.disable();
  _dmaComplete = true;
}

bool SSD1322_DMA::isRunning() const {
  return !_dmaComplete;
}

// --------------------
// *--DMA Prototypes--*
// --------------------

// Wait for SPI transmission to complete
void SSD1322_DMA::waitTransmitComplete() {
  uint32_t tmp __attribute__((unused));
  
  // Wait until all bytes have been received
  while (_pendingRxCount) {
    if ((LPSPI4_RSR & LPSPI_RSR_RXEMPTY) == 0) {
      tmp = LPSPI4_RDR;  // Read any pending RX bytes
      _pendingRxCount--; // decrement count of bytes still left
    }
    
    // Add a timeout mechanism
    static unsigned long timeout_start = 0;
    if (timeout_start == 0) timeout_start = millis();
    if (millis() - timeout_start > 1000) {
      Serial.println("waitTransmitComplete timeout!");
      break;
    }
  }
  
  // Clear RX FIFO
  LPSPI4_CR = LPSPI_CR_MEN | LPSPI_CR_RRF;
}

// Setup display window for data transfer
void SSD1322_DMA::setup_display_window() {
  if (!_disp) return;
  
  // Set up the display window
  _disp->api.SSD1322_API_set_window(0, DISPLAY_WIDTH/4 - 1, 0, DISPLAY_HEIGHT - 1);
  _disp->api.SSD1322_API_command(SSD1322_WRITE_RAM);
}

// Pack pixels from source buffer to packed buffer (4 bits per pixel format)
void SSD1322_DMA::pack_pixels(uint8_t *src_buffer, uint8_t *packed_buffer, int size) {
  for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i += 2) {
    uint8_t a = src_buffer[i];
    uint8_t b = src_buffer[i+1];
    packed_buffer[i/2] = ((a & 0x0F) << 4) | (b & 0x0F);
  }
}

// Break the framebuffer into chunks for interruptible processing
bool SSD1322_DMA::draw_framebuffer_interruptible(uint8_t *src_buffer) {
  if (!_disp) {
    Serial.println("ERROR: Display not initialized. Call init() first.");
    return true; // Return true to indicate we're "done"
  }
  
  static uint8_t packed_buffer[FRAMEBUFFER_SIZE/2];
  static int currentChunk = 0;
  static bool transferInProgress = false;
  bool done = false;
  
  // Initial setup - pack pixels and send commands just once
  if (!transferInProgress) {
    // Pack the framebuffer (convert to 4bpp format that display expects)
    pack_pixels(src_buffer, packed_buffer, FRAMEBUFFER_SIZE/2);
    
    // Set up the display window
    setup_display_window();
    
    transferInProgress = true;
    currentChunk = 0;
  }
  
  // Transfer one chunk at a time
  if (currentChunk < NUM_CHUNKS) {
    int startOffset = currentChunk * TRANSFER_CHUNK_SIZE;
    int bytesToTransfer = min(TRANSFER_CHUNK_SIZE, 
                          FRAMEBUFFER_SIZE/2 - startOffset);
    
    // Send this chunk of data
    for (int i = 0; i < bytesToTransfer; i++) {
      _disp->api.SSD1322_API_data(packed_buffer[startOffset + i]);
    }
    
    currentChunk++;
  } else {
    // All chunks processed, reset for next frame
    transferInProgress = false;
    done = true;
  }
  
  return done;
}

// DMA circular ISR - updated to be consistent with minimal_dma_example.ino
void SSD1322_DMA::dmaCircularCompleteISR() {
  // Clear interrupt flag
  _dma.clearInterrupt();
  
  // Increment trigger counter
  _dmaTriggerCount++;
  
  // Check if we're no longer in continuous mode
  if (!_continuousMode) {
    // Disable DMA if not in continuous mode
    LPSPI4_DER = 0;
    _dma.disable();
    _dmaComplete = true;
  }
}

void SSD1322_DMA::setupDmamuxChannel() {
  // Reset DMAMUX completely to ensure clean state
  volatile uint32_t *chcfg = &DMAMUX_CHCFG0 + _dma.channel;
  *chcfg = 0;  // Disable channel completely
  delayMicroseconds(1);
}

void SSD1322_DMA::configureDmaChain() {
  // Initialize DMA channel
  _dma.begin(true);
  
  // Configure DMA channel to use LPSPI4_TX trigger
  int ch = _dma.channel;
  
  // Enable DMA clock
  CCM_CCGR5 |= CCM_CCGR5_DMA_MASK;
  
  // Configure NVIC for DMA interrupt
  NVIC_ENABLE_IRQ(IRQ_DMA_CH0 + ch);
  
  // Configure the DMA to circle within the framebuffer
  static uint8_t framebuffer[FRAMEBUFFER_SIZE] __attribute__((aligned(32)));
  
  // Initialize framebuffer with test pattern or zeroes
  for (int i = 0; i < FRAMEBUFFER_SIZE; i++) {
    framebuffer[i] = 0;
  }
  
  _dma.sourceBuffer(framebuffer, FRAMEBUFFER_SIZE);
  _dma.destination(LPSPI4_TDR);
  _dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit transfer
  _dma.TCD->NBYTES = 1;     // Transfer 1 byte at a time
  _dma.TCD->DOFF = 0;       // Don't increment destination
  
  // Attach interrupt handler
  _dma.attachInterrupt(dma_circular_complete_isr);
  
  // Setup for continuous mode
  _pendingRxCount = FRAMEBUFFER_SIZE;
}

void SSD1322_DMA::configureSpiFifos() {
  // Reset and configure LPSPI
  LPSPI4_CR &= ~LPSPI_CR_MEN;  // Disable module first
  LPSPI4_CR |= LPSPI_CR_RTF | LPSPI_CR_RRF;  // Reset TX and RX FIFOs
  LPSPI4_FCR = 0;  // Zero watermark
  LPSPI4_CR |= LPSPI_CR_MEN;  // Re-enable module
  
  // Configure LPSPI for transfers
  LPSPI4_TCR = LPSPI_TCR_FRAMESZ(7) | (1 << 22);  // 8-bit, CONT=1
  LPSPI4_CFGR1 = LPSPI_CFGR1_MASTER | (1 << 5) | LPSPI_CFGR1_NOSTALL;
  LPSPI4_SR = 0x3F00;  // Clear errors
  LPSPI4_DER = LPSPI_DER_TDDE;  // Enable TX DMA
}

void SSD1322_DMA::primeTxFifoAndStartDma() {
  // Configure hardware trigger for DMA
  _dma.triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX);
  
  // Direct register access for DMAMUX with proper enable bit
  volatile uint32_t *chcfg = &DMAMUX_CHCFG0 + _dma.channel;
  *chcfg = DMAMUX_SOURCE_LPSPI4_TX | DMAMUX_CHCFG_ENBL_MASK;
  
  // Prime the first transfer
  LPSPI4_TDR = 0;  // Send first byte to start DMA
  
  // Start DMA
  _dma.enable();
  _dmaComplete = false;
  _continuousMode = true;
}

// ISR function called from DMA interrupt
void dma_circular_complete_isr() {
  // Call the class method
  dmaDriver.dmaCircularCompleteISR();
}

#endif  



