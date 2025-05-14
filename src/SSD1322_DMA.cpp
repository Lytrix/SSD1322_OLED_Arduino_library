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

// DMA circular ISR
void SSD1322_DMA::dmaCircularCompleteISR() {
  // This is called when a complete segment has been transmitted
  
  // Clear the interrupt flag
  _dma.clearInterrupt();
  
  // Clear DONE bit (critical for continuous operation)
  _dma.TCD->CSR = _dma.TCD->CSR & ~DMA_TCD_CSR_DONE;
  
  // Ensure DMA is still in ERQ
  DMA_SERQ = _dma.channel;  // Set request again
  
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
  // Reset DMA for a fresh setup and disable any active settings
  _dma.disable();
  
  // Reset DMAMUX completely to ensure clean state
  DMAMUX_CHCFG0 = 0;  // Disable channel completely
  delayMicroseconds(1);
}

void SSD1322_DMA::configureDmaChain() {
  // Initialize DMA channel
  _dma.begin(true);
  _pendingRxCount = FRAMEBUFFER_SIZE;
  
  // We need a real frame buffer to configure DMA settings
  // This is a simplification - in a real app, you'd use your actual frame buffer
  static uint8_t dummyBuffer[FRAMEBUFFER_SIZE];
  
  // Configure the three segments of the circular DMA buffer
  _dmaSettings[0].sourceBuffer(dummyBuffer, SEGMENT_SIZE);
  _dmaSettings[0].destination(LPSPI4_TDR);
  _dmaSettings[0].transferSize(1);  // 8-bit transfers
  _dmaSettings[0].transferCount(SEGMENT_SIZE);
  _dmaSettings[0].replaceSettingsOnCompletion(_dmaSettings[1]);
  _dmaSettings[0].TCD->CSR &= ~DMA_TCD_CSR_DREQ; // Don't disable channel on completion
  
  _dmaSettings[1].sourceBuffer(dummyBuffer + SEGMENT_SIZE, SEGMENT_SIZE);
  _dmaSettings[1].destination(LPSPI4_TDR);
  _dmaSettings[1].transferSize(1);
  _dmaSettings[1].transferCount(SEGMENT_SIZE);
  _dmaSettings[1].replaceSettingsOnCompletion(_dmaSettings[2]);
  _dmaSettings[1].TCD->CSR &= ~DMA_TCD_CSR_DREQ; // Don't disable channel on completion
  
  _dmaSettings[2].sourceBuffer(dummyBuffer + 2 * SEGMENT_SIZE, SEGMENT_SIZE);
  _dmaSettings[2].destination(LPSPI4_TDR);
  _dmaSettings[2].transferSize(1);
  _dmaSettings[2].transferCount(SEGMENT_SIZE);
  _dmaSettings[2].replaceSettingsOnCompletion(_dmaSettings[0]);  // Circular: Back to first segment
  _dmaSettings[2].interruptAtCompletion();  // Trigger interrupt at end of this segment
  _dmaSettings[2].TCD->CSR &= ~DMA_TCD_CSR_DREQ; // Don't disable channel on completion
  
  // Configure main DMA channel with first segment settings
  _dma = _dmaSettings[0];
  _dma.attachInterrupt(dma_circular_complete_isr);
}

void SSD1322_DMA::configureSpiFifos() {
  // Reset FIFO and SPI module first
  LPSPI4_CR = LPSPI_CR_RST;  // Reset SPI module
  delayMicroseconds(1);
  LPSPI4_CR = 0;
  delayMicroseconds(1);
  
  // Configure optimal FIFO watermarks
  LPSPI4_FCR = LPSPI_FCR_TXWATER(1) | LPSPI_FCR_RXWATER(3);
  
  // Clear status flags
  LPSPI4_SR = 0x3F00;  // Clear all status flags
  
  // Reset FIFOs
  LPSPI4_CR |= LPSPI_CR_RTF | LPSPI_CR_RRF;
  delayMicroseconds(1);

  // Enable SPI module
  LPSPI4_CR = LPSPI_CR_MEN;
  delayMicroseconds(1);
  
  // Make sure RX FIFO is empty
  while ((LPSPI4_RSR & LPSPI_RSR_RXEMPTY) == 0) {
    volatile uint32_t dummy = LPSPI4_RDR;
    (void)dummy;
  }
}

void SSD1322_DMA::primeTxFifoAndStartDma() {
  // Set the DMAMUX to trigger DMA channel from LPSPI4_TX
  DMAMUX_CHCFG0 = 0;  // Disable first
  delayMicroseconds(1);
  DMAMUX_CHCFG0 = DMAMUX_SOURCE_LPSPI4_TX | DMAMUX_ENABLE;
  
  // Update TCR register - ensure 8-bit transfers
  LPSPI4_TCR = 7;  // 8-bit transfers (FRAMESZ=7 for 8 bits)
  
  // Reset FIFOs one more time before starting
  LPSPI4_CR |= LPSPI_CR_RTF | LPSPI_CR_RRF;
  delayMicroseconds(1);
  
  // Start DMA in critical section to prevent interruptions
  noInterrupts();
  
  // Prime the TX FIFO with a few bytes to get transfers started
  LPSPI4_TDR = 0;  // Dummy data
  LPSPI4_TDR = 0;
  delayMicroseconds(1);
  
  // Clear DONE bit
  _dma.TCD->CSR &= ~DMA_TCD_CSR_DONE;
  
  // Enable request and hardware
  DMA_SERQ = _dma.channel;
  
  // Enable TX DMA first, then RX DMA
  LPSPI4_DER = LPSPI_DER_TDDE;  // TX DMA enable first
  delayMicroseconds(1);
  LPSPI4_DER |= LPSPI_DER_RDDE; // RX DMA enable second
  
  interrupts();
}

// ISR function called from DMA interrupt
void dma_circular_complete_isr() {
  // Call the class method
  dmaDriver.dmaCircularCompleteISR();
}

#endif  



