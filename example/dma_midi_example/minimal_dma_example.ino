#include <SPI.h>
#include <DMAChannel.h>
#include <stdlib.h>  // For malloc

// Pin definitions for SPI
#define PIN_CS    10
#define PIN_DC    9
#define PIN_RESET 8

// DMAMUX constants
#define DMAMUX_ENABLE 0x80
// Use the built-in definition from imxrt.h instead of redefining
// #define DMAMUX_SOURCE_LPSPI4_TX  0x50  // Source 80 (0x50) for LPSPI4_TX
#define DMAMUX_CHCFG_ENBL_MASK   (1U << 31)    // Enable bit is at bit 31

// Clock control registers
#define SIM_SCGC6_DMAMUX_MASK    (1U << 4)     // DMAMUX clock gate control bit
#define CCM_CCGR5_DMA_MASK       (3U << 2)     // DMA clock gate control bits

// Test buffer size
#define BUFFER_SIZE    2048
#define SEGMENT_SIZE   32  // Transfer 32 bytes at a time

// Add these definitions at the top
#define FRAMEBUFFER_SIZE 8192
#define NUM_TCD 4  // Split into 4 TCDs of 2048 bytes each
#define TCD_SIZE (FRAMEBUFFER_SIZE / NUM_TCD)

// Forward declarations
void dmaInterruptHandler();
void startDMATransfer();


// DMA state
DMAChannel dma(0);  // Explicitly request channel 0
volatile uint8_t buffer1[BUFFER_SIZE] __attribute__((aligned(32)));
volatile uint8_t buffer2[BUFFER_SIZE] __attribute__((aligned(32)));
volatile uint8_t *currentBuffer = buffer1;
volatile uint8_t *nextBuffer = buffer2;
volatile bool dmaActive = false;
volatile bool dmaComplete = false;
volatile uint32_t frameCount = 0;
volatile uint32_t lastFrameTime = 0;
volatile uint32_t currentSegment = 0;  // Track current segment being transferred

// Define DMA TCD array and framebuffer
DMAChannel* dma_channels[NUM_TCD];  // Array of DMA channels
volatile uint8_t framebuffer[FRAMEBUFFER_SIZE] __attribute__((aligned(32)));

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(1000);  // Longer delay to ensure serial is ready
  Serial.println("\n\n\n");  // Clear any garbage
  Serial.println("=================================");
  Serial.println("Teensy 4.1 SPI DMA Test Starting");
  Serial.println("=================================");
  
  // Test serial communication
  for (int i = 0; i < 5; i++) {
    Serial.printf("Serial test %d\n", i);
    delay(100);
  }
  
  // Test 1: Basic pin operations
  Serial.println("\nTest 1: Basic pin operations");
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  Serial.println("LED test complete");
  
  // Test 2: SPI initialization
  Serial.println("\nTest 2: SPI initialization");
  Serial.println("Starting SPI.begin()...");
  SPI.begin();
  Serial.println("SPI.begin() completed");
  delay(100);  // Add delay after SPI initialization
  
  // Test SPI pins
  Serial.println("Starting pin configuration...");
  Serial.println("Configuring CS pin...");
  pinMode(PIN_CS, OUTPUT);
  Serial.println("CS pin configured");
  delay(50);  // Add delay between pin configurations
  
  Serial.println("Configuring DC pin...");
  pinMode(PIN_DC, OUTPUT);
  Serial.println("DC pin configured");
  delay(50);
  
  Serial.println("Configuring RESET pin...");
  pinMode(PIN_RESET, OUTPUT);
  Serial.println("RESET pin configured");
  delay(50);
  
  // Test pin operations one at a time
  Serial.println("Starting pin operations test...");
  
  Serial.println("Testing CS pin...");
  digitalWrite(PIN_CS, HIGH);
  delay(50);
  digitalWrite(PIN_CS, LOW);
  delay(50);
  digitalWrite(PIN_CS, HIGH);
  Serial.println("CS pin test complete");
  delay(50);
  digitalWrite(PIN_CS, LOW);
  
  Serial.println("Testing DC pin...");
  digitalWrite(PIN_DC, HIGH);
  delay(50);
  digitalWrite(PIN_DC, LOW);
  delay(50);
  digitalWrite(PIN_DC, HIGH);
  Serial.println("DC pin test complete");
  delay(50);
  
  Serial.println("Testing RESET pin...");
  digitalWrite(PIN_RESET, HIGH);
  delay(50);
  digitalWrite(PIN_RESET, LOW);
  delay(50);
  digitalWrite(PIN_RESET, HIGH);
  Serial.println("RESET pin test complete");
  delay(50);
  
  Serial.println("Pin operations test complete");
  delay(100);  // Add delay before next test
  
  // Test 3: Buffer initialization
  Serial.println("\nTest 3: Buffer initialization");
  delay(100);  // Add delay before buffer initialization
  
  Serial.println("Checking buffer memory...");
  if (buffer1 == nullptr || buffer2 == nullptr) {
    Serial.println("ERROR: Buffer allocation failed!");
    while(1) { delay(1000); }
  }
  Serial.println("Buffer memory check passed");
  delay(50);
  
  Serial.println("Starting buffer1 initialization...");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer1[i] = i & 0xFF;
    if (i % 512 == 0) {  // Report progress more frequently
      Serial.printf("Buffer1 progress: %d%% (%d/%d bytes)\n", 
                   (i * 100) / BUFFER_SIZE, i, BUFFER_SIZE);
      delay(10);  // Small delay during progress reporting
    }
  }
  Serial.println("Buffer1 initialization complete");
  delay(50);
  
  Serial.println("Starting buffer2 initialization...");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer2[i] = (~(i & 0xFF)) & 0xFF;  // Ensure unsigned result
    if (i % 512 == 0) {  // Report progress more frequently
      Serial.printf("Buffer2 progress: %d%% (%d/%d bytes)\n", 
                   (i * 100) / BUFFER_SIZE, i, BUFFER_SIZE);
      delay(10);  // Small delay during progress reporting
    }
  }
  Serial.println("Buffer2 initialization complete");
  
  // Verify buffer contents
  Serial.println("Verifying buffer contents...");
  bool buffer1_ok = true;
  bool buffer2_ok = true;
  
  for (int i = 0; i < BUFFER_SIZE; i += 1024) {  // Check every 1024th byte
    if (buffer1[i] != (i & 0xFF)) {
      Serial.printf("Buffer1 verification failed at index %d: expected %d, got %d\n",
                   i, i & 0xFF, buffer1[i]);
      buffer1_ok = false;
      break;
    }
    if (buffer2[i] != ((~(i & 0xFF)) & 0xFF)) {  // Ensure unsigned comparison
      Serial.printf("Buffer2 verification failed at index %d: expected %d, got %d\n",
                   i, ((~(i & 0xFF)) & 0xFF), buffer2[i]);
      buffer2_ok = false;
      break;
    }
  }
  
  if (buffer1_ok && buffer2_ok) {
    Serial.println("Buffer verification passed");
  } else {
    Serial.println("Buffer verification failed!");
  }
  
  delay(100);  // Add delay before next test
  
  // Test 4: DMA clock and basic setup
  Serial.println("\nTest 4: DMA clock and basic setup");
  delay(100);  // Add delay before DMA setup
  
  Serial.println("Checking CCM_CCGR5 register...");
  Serial.printf("CCM_CCGR5 before: %08X\n", CCM_CCGR5);
  
  Serial.println("Enabling DMA clock...");
  // Enable DMA clock in CCM_CCGR5
  CCM_CCGR5 |= CCM_CCGR5_DMA_MASK;
  Serial.printf("CCM_CCGR5 after: %08X\n", CCM_CCGR5);
  
  // Verify DMA clock is enabled
  if ((CCM_CCGR5 & CCM_CCGR5_DMA_MASK) != CCM_CCGR5_DMA_MASK) {
    Serial.println("ERROR: Failed to enable DMA clock!");
    while(1) { delay(1000); }
  }
  Serial.println("DMA clock enabled and verified");
  delay(50);
  
  Serial.println("Initializing DMA channel...");
  Serial.println("Calling dma.begin(true)...");
  dma.begin(true);
  Serial.println("DMA channel initialized");
  delay(50);
  
  int ch = dma.channel;
  Serial.printf("Using DMA channel %d\n", ch);
  
  // Print initial DMA state
  Serial.println("Initial DMA TCD configuration:");
  Serial.printf("SADDR: %p\n", dma.TCD->SADDR);
  Serial.printf("SOFF: %d\n", dma.TCD->SOFF);
  Serial.printf("ATTR: 0x%02X\n", dma.TCD->ATTR);
  Serial.printf("NBYTES: %d\n", dma.TCD->NBYTES);
  Serial.printf("SLAST: %d\n", dma.TCD->SLAST);
  Serial.printf("DADDR: %p\n", dma.TCD->DADDR);
  Serial.printf("DOFF: %d\n", dma.TCD->DOFF);
  Serial.printf("CITER: %d\n", dma.TCD->CITER);
  Serial.printf("BITER: %d\n", dma.TCD->BITER);
  Serial.printf("CSR: 0x%04X\n", dma.TCD->CSR);
  
  delay(100);  // Add delay before next test
  
  // Test 5: DMAMUX configuration
  Serial.println("\nTest 5: DMAMUX configuration");
  delay(100);  // Add delay before DMAMUX setup
  
  // Verify DMA channel is valid
  if (ch < 0 || ch > 7) {
    Serial.printf("ERROR: Invalid DMA channel %d\n", ch);
    while(1) { delay(1000); }
  }
  Serial.printf("DMA channel %d is valid\n", ch);
  
  // Print current DMAMUX configuration
  Serial.println("Current DMAMUX configuration:");
  for (int i = 0; i < 8; i++) {
    Serial.printf("DMAMUX_CHCFG%d: %08X\n", i, DMAMUX_CHCFG0 + i);
  }
  
  Serial.println("Configuring DMAMUX...");
  // Direct register access for DMAMUX with proper enable bit
  volatile uint32_t *chcfg = &DMAMUX_CHCFG0 + ch;
  *chcfg = DMAMUX_SOURCE_LPSPI4_TX | DMAMUX_CHCFG_ENBL_MASK;  // Use proper enable mask
  Serial.printf("DMAMUX_CHCFG%d: %08X\n", ch, *chcfg);
  
  // Verify DMAMUX configuration
  uint32_t actual = *chcfg;
  uint32_t expected = DMAMUX_SOURCE_LPSPI4_TX | DMAMUX_CHCFG_ENBL_MASK;
  if (actual != expected) {
    Serial.println("ERROR: DMAMUX configuration failed!");
    Serial.printf("Expected: 0x%08X, Actual: 0x%08X\n", expected, actual);
    while(1) { delay(1000); }
  }
  Serial.println("DMAMUX configured and verified");
  
  Serial.println("Configuring NVIC...");
  // Configure NVIC for DMA interrupt
  NVIC_ENABLE_IRQ(IRQ_DMA_CH0 + ch);
  Serial.printf("NVIC_ISER0: %08X\n", NVIC_ISER0);
  Serial.printf("NVIC_ICER0: %08X\n", NVIC_ICER0);
  Serial.println("NVIC configured");
  
  // Attach interrupt handler
  dma.attachInterrupt(dmaInterruptHandler);
  Serial.println("DMA interrupt handler attached");
  
  // Verify interrupt configuration
  Serial.printf("DMA TCD CSR after attach: %04X\n", dma.TCD->CSR);
  Serial.printf("NVIC_ISER0 after attach: %08X\n", NVIC_ISER0);
  
  Serial.println("Starting SPI configuration...");
  // Configure SPI for DMA
  SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
  Serial.println("SPI transaction started");
  
  // Setup scatter-gather DMA
  setupScatterGatherDMA();
}

void loop() {
  // Drain RX FIFO to prevent overflow - minimal CPU usage (<1 microsecond)
  if (LPSPI4_SR & LPSPI_SR_RDF) {
    (void)LPSPI4_RDR;
  }
  
  static uint32_t lastTime = 0;
  uint32_t currentTime = millis();
  
  if (currentTime - lastTime > 1000) {
    lastTime = currentTime;
    
    // Start CPU usage timing
    uint32_t startMicros = micros();
    
    // Clear any error flags - minimal CPU usage
    if (LPSPI4_SR & 0x3F00) {
      LPSPI4_SR = 0x3F00;
    }
    
    // Animation effect - this is the only real CPU work
    static uint8_t pattern = 0;
    pattern++;
    
    // Measure time for framebuffer update
    for (int i = 0; i < FRAMEBUFFER_SIZE; i += 64) {
      framebuffer[i] = pattern;
    }
    
    uint32_t elapsedMicros = micros() - startMicros;
    
    // Print debug info
    Serial.println("\nDMA Status:");
    Serial.printf("  SADDR: %p\n", dma_channels[0]->TCD->SADDR);
    Serial.printf("  CITER: %d\n", dma_channels[0]->TCD->CITER);
    Serial.printf("  CSR: 0x%04X\n", dma_channels[0]->TCD->CSR);
    Serial.printf("LPSPI_SR: 0x%08X\n", LPSPI4_SR);
    Serial.printf("LPSPI_FSR: 0x%08X (TX: %d, RX: %d)\n", 
                  LPSPI4_FSR, LPSPI4_FSR & 0x0F, (LPSPI4_FSR >> 4) & 0x0F);
    Serial.printf("CPU usage for framebuffer update: %d microseconds\n", elapsedMicros);
    
    // Calculate % CPU usage for DMA-related operations
    float cpuUsagePercent = (elapsedMicros / 1000000.0f) * 100.0f;
    Serial.printf("CPU usage: %.5f%% (rest is free)\n", cpuUsagePercent);
  }
  
  // Do other CPU-intensive tasks here - the CPU is completely free!
  // For example, MIDI processing would go here and have full CPU availability
}

// New setup function to configure scatter-gather DMA
void setupScatterGatherDMA() {
  Serial.println("1. Setting up single channel DMA...");
  
  // Configure SPI
  SPI.beginTransaction(SPISettings(30000000, MSBFIRST, SPI_MODE0));
  
  Serial.println("2. Configuring LPSPI registers...");
  // Reset and configure LPSPI
  LPSPI4_CR &= ~LPSPI_CR_MEN;
  LPSPI4_CR |= LPSPI_CR_RTF | LPSPI_CR_RRF;
  LPSPI4_FCR = 0;  // Zero watermark
  LPSPI4_CR |= LPSPI_CR_MEN;
  
  // Configure LPSPI
  LPSPI4_TCR = LPSPI_TCR_FRAMESZ(7) | (1 << 22);  // 8-bit, CONT=1
  LPSPI4_CFGR1 = LPSPI_CFGR1_MASTER | (1 << 5) | LPSPI_CFGR1_NOSTALL;
  LPSPI4_SR = 0x3F00;  // Clear errors
  LPSPI4_DER = LPSPI_DER_TDDE;  // Enable TX DMA
  
  Serial.println("3. Initializing framebuffer...");
  // Initialize framebuffer with test pattern
  for (int i = 0; i < FRAMEBUFFER_SIZE; i++) {
    framebuffer[i] = i & 0xFF;
  }
  
  Serial.println("4. Creating single DMA channel...");
  // Create a single DMA channel
  dma_channels[0] = new DMAChannel();
  if (dma_channels[0] == nullptr) {
    Serial.println("ERROR: Failed to create DMA channel!");
    while(1) delay(1000);
  }
  dma_channels[0]->begin(true);
  
  Serial.println("5. Configuring DMA...");
  // Configure the DMA to circle within the framebuffer
  dma_channels[0]->sourceBuffer(framebuffer, FRAMEBUFFER_SIZE);
  dma_channels[0]->destination(LPSPI4_TDR);
  dma_channels[0]->TCD->ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit transfer
  dma_channels[0]->TCD->NBYTES = 1;     // Transfer 1 byte at a time
  dma_channels[0]->TCD->DOFF = 0;       // Don't increment destination
  // SLAST is set automatically by sourceBuffer
  
  Serial.println("6. Setting up hardware trigger...");
  // Configure hardware trigger
  dma_channels[0]->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX);
  
  Serial.println("7. Setting up CS pin...");
  // Set up CS pin
  digitalWrite(PIN_CS, LOW);
  
  Serial.println("8. Priming first transfer...");
  // Prime the first transfer
  LPSPI4_TDR = framebuffer[0];
  
  // Let's also fix the unused variable warning in the loop function
  Serial.println("9. Enabling DMA...");
  // Start DMA
  dma_channels[0]->enable();
  
  Serial.println("DMA transfer started");
}

// DMA interrupt handler
void dmaInterruptHandler() {
  // Clear interrupt flag
  dma.clearInterrupt();
  
  // Detailed debug info
  Serial.println("\nDMA Interrupt Handler Called!");
  Serial.printf("DMA State:\n");
  Serial.printf("  CITER: %d, BITER: %d\n", dma.TCD->CITER, dma.TCD->BITER);
  Serial.printf("  CSR: 0x%04X\n", dma.TCD->CSR);
  Serial.printf("  SADDR: %p\n", dma.TCD->SADDR);
  Serial.printf("  DADDR: %p\n", dma.TCD->DADDR);
  
  Serial.printf("LPSPI State:\n");
  Serial.printf("  SR: 0x%08X\n", LPSPI4_SR);
  Serial.printf("  DER: 0x%08X\n", LPSPI4_DER);
  Serial.printf("  FCR: 0x%08X\n", LPSPI4_FCR);
  Serial.printf("  FSR: 0x%08X (TX: %d, RX: %d)\n", 
               LPSPI4_FSR, 
               LPSPI4_FSR & 0x0F, 
               (LPSPI4_FSR >> 4) & 0x0F);
  Serial.printf("  TCR: 0x%08X\n", LPSPI4_TCR);
  Serial.printf("  CFGR1: 0x%08X\n", LPSPI4_CFGR1);
  
  // In scatter-gather mode, we don't need to manually start the next segment
  // as the DMA engine automatically chains to the next TCD
  
  // Update statistics
  frameCount++;
  uint32_t now = millis();
  uint32_t frameDuration = now - lastFrameTime;
  lastFrameTime = now;
  
  Serial.printf("DMA Interrupt complete! Duration: %dms\n", frameDuration);
} 