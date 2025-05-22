#include <Arduino.h>
#include <SPI.h>
#include "SSD1322.h"
// #include "SSD1322_DMA.h"
#include <SSD1322_DMA.h>

// Configuration constants
#define DMABUFFER_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 2)  // 4bpp packed pixels

DMAMEM uint8_t dma_buffer[DMABUFFER_SIZE] __attribute__((aligned(32)));

// Pin definitions (adjust according to your setup)
#define OLED_CS_PIN    10
#define OLED_DC_PIN    9
#define OLED_HEIGHT    64 // Set to your display's height
#define OLED_WIDTH     256 // Set to your display's width
#define OLED_RESET_PIN 8

extern volatile bool dmaDoneFlag;

// Forward declarations
void dmaCallback();
void testSpiMode();
void testDmaMode();
void showBlankScreen();
void testManualFifoFill();

// Create instances
SSD1322 oled(OLED_CS_PIN, OLED_DC_PIN, OLED_HEIGHT, OLED_WIDTH, OLED_RESET_PIN);

// Add this at the top, after includes and before setup():
// uint8_t dma_buffer[(OLED_WIDTH / 2) * OLED_HEIGHT];

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Wait for Serial
  }
  Serial.println("\n\n----- SSD1322 DMA Test -----");
  
  // Configure OLED pins
  pinMode(OLED_CS_PIN, OUTPUT);
  pinMode(OLED_DC_PIN, OUTPUT);
  pinMode(OLED_RESET_PIN, OUTPUT);


  // Setup SPI
  SPI.begin();
  
  
    // Initialize display
  oled.api.SSD1322_API_init();
  // If you have a different init, use it here.
  oled.gfx.set_buffer_size(256, 64);
  
  // Reset the display
  // digitalWrite(OLED_RESET_PIN, LOW);
  // delay(10);
  // digitalWrite(OLED_RESET_PIN, HIGH);
  // delay(100);
  


  // Initialize SSD1322_DMA
  SSD1322_DMA0.begin(OLED_CS_PIN, SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  // Short delay for everything to initialize
  delay(100);
  
  Serial.println("Setup complete, starting tests...");
}

void loop() {
  // Test regular SPI transfers first
  Serial.println("\n--- Standard SPI Test ---");
  testSpiMode();
  delay(2000);

  // Show blank screen between tests
  // Serial.println("\n--- Blank Screen ---");
  // showBlankScreen();

  // Manual FIFO fill test
  // testManualFifoFill();
  // delay(2000);

  // Test DMA mode
  Serial.println("\n--- DMA Test ---");
  testDmaMode();
  delay(2000);

  // Try the interruptible draw method
  // Serial.println("\n--- Interruptible DMA Test ---");
  // bool completed = false;
  // while (!completed) {
  //   completed = oledDma.draw_framebuffer_interruptible(dma_buffer);
  //   delay(10);
  // }
  // delay(2000);

  Serial.println("All tests completed, waiting 10 seconds...");
  delay(5000);
}

// DMA transfer complete callback
void dmaCallback() {
  static int callCount = 0;
  callCount++;
  
  if (callCount % 10 == 0) {
    Serial.printf("DMA callback fired %d times\n", callCount);
  }
}

// Test with regular SPI transfers
void testSpiMode() {
  // Clear the display
  // oled.api.SSD1322_API_clear();
  delay(50);
  
  // Set up window for drawing
  oled.api.SSD1322_API_set_window(0, 127, 0, 63);
  oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
  // Send test pattern through SPI
  digitalWrite(OLED_DC_PIN, HIGH); // Data mode
  digitalWrite(OLED_CS_PIN, LOW);  // Select display
  
  // Debug: Print SPI config before
    Serial.println("SPI/DMA Config AFTER:");
    Serial.printf("LPSPI4_CR = 0x%08X\n", LPSPI4_CR);
    Serial.printf("LPSPI4_FCR = 0x%08X\n", LPSPI4_FCR);
    Serial.printf("LPSPI4_TCR = 0x%08X\n", LPSPI4_TCR);
    Serial.printf("LPSPI4_CFGR1 = 0x%08X\n", LPSPI4_CFGR1);
    Serial.printf("LPSPI4_DER = 0x%08X\n", LPSPI4_DER);
    Serial.printf("LPSPI4_FSR = 0x%08X (TX: %d, RX: %d)\n", 
                 LPSPI4_FSR, LPSPI4_FSR & 0x0F, (LPSPI4_FSR >> 4) & 0x0F);

  // Send test pattern
  uint32_t startTime = micros();
  
  for (int y = 0; y < OLED_HEIGHT; y++) {
    for (int x = 0; x < OLED_WIDTH; x++) {
        uint8_t left = ((x * 2) * 15) / (OLED_WIDTH - 1);
        uint8_t right = ((x * 2 + 1) * 15) / (OLED_WIDTH - 1);
        SPI.transfer((left << 4) | right);
    }
  }
  
   // Debug: Print SPI config after 
    Serial.println("SPI/DMA Config AFTER:");
    Serial.printf("LPSPI4_CR = 0x%08X\n", LPSPI4_CR);
    Serial.printf("LPSPI4_FCR = 0x%08X\n", LPSPI4_FCR);
    Serial.printf("LPSPI4_TCR = 0x%08X\n", LPSPI4_TCR);
    Serial.printf("LPSPI4_CFGR1 = 0x%08X\n", LPSPI4_CFGR1);
    Serial.printf("LPSPI4_DER = 0x%08X\n", LPSPI4_DER);
    Serial.printf("LPSPI4_FSR = 0x%08X (TX: %d, RX: %d)\n", 
                 LPSPI4_FSR, LPSPI4_FSR & 0x0F, (LPSPI4_FSR >> 4) & 0x0F);

  uint32_t endTime = micros();
  digitalWrite(OLED_CS_PIN, HIGH); // Deselect display
  
  Serial.printf("SPI transfer time: %u microseconds for 1024 bytes\n", endTime - startTime);
}

// Test with DMA transfers
void testDmaMode() {
  // Measure time to fill buffer (CPU busy)
  uint32_t fillStart = micros();
  oled.gfx.fill_buffer(dma_buffer, 0xff);
  uint32_t fillEnd = micros();
  Serial.printf("Time to fill buffer (CPU busy): %u microseconds\n", fillEnd - fillStart);

  // Flush cache before DMA
  arm_dcache_flush((void*)dma_buffer, DMABUFFER_SIZE);

  // Set up window
  oled.api.SSD1322_API_set_window(0, 63, 0, 63);
  oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
  digitalWrite(OLED_DC_PIN, HIGH);
  digitalWrite(OLED_CS_PIN, LOW);

  // Measure CPU busy time for DMA setup (non-blocking)
  uint32_t cpuStart = micros();
  SSD1322_DMA0.queue(dma_buffer, DMABUFFER_SIZE);
  uint32_t cpuEnd = micros();
  Serial.printf("CPU busy time for DMA setup: %u microseconds\n", cpuEnd - cpuStart);

  // Measure total elapsed time for DMA transfer (CPU is free during this time)
  uint32_t dmaStart = micros();
  while (SSD1322_DMA0.remained() > 0) {
    // Optionally do other work here
    // yield(); // Uncomment if you want to yield
  }
  uint32_t dmaEnd = micros();
  digitalWrite(OLED_CS_PIN, HIGH);
  Serial.printf("Total elapsed time for DMA transfer (CPU free): %u microseconds\n", dmaEnd - dmaStart);
}

// void showBlankScreen() {
//   // Set up window for drawing
  
//   oled.api.SSD1322_API_set_window(0, 127, 0, 63);
//   oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
//   digitalWrite(OLED_DC_PIN, HIGH);
//   digitalWrite(OLED_CS_PIN, LOW);

//   for (int y = 0; y < OLED_HEIGHT; y++) {
//     for (int x = 0; x < OLED_WIDTH/2; x++) {
//       SPI.transfer(0x00);
      
//     }
//   }
 
//   digitalWrite(OLED_CS_PIN, HIGH);
//   delay(2000); // Show blank screen for 2s
// }

void testManualFifoFill() {
  Serial.println("\n--- Manual FIFO Fill Test ---");
  // Set up window for drawing
  oled.api.SSD1322_API_set_window(0, OLED_HEIGHT - 1, 0, (OLED_WIDTH / 2) - 1);
  oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
  digitalWrite(OLED_DC_PIN, HIGH);
  digitalWrite(OLED_CS_PIN, LOW);

  // Use the same TCR as in DMA
  //LPSPI4_TCR = LPSPI_TCR_FRAMESZ(7);// | (0 << 16) | (1 << 22); // 8-bit, PCS=0, CONT=1
   LPSPI4_TCR = LPSPI_TCR_FRAMESZ(7) | (0 << 16) ; // 8-bit, PCS=0, CONT=1
  // Write up to 8 bytes to the FIFO
  for (int i = 0; i < 4; i++) {
    LPSPI4_TDR = 0xFF;
  }
  Serial.printf("FSR after manual fill: 0x%08X (TX: %d, RX: %d)\n", LPSPI4_FSR, LPSPI4_FSR & 0x0F, (LPSPI4_FSR >> 4) & 0x0F);

  // Wait for FIFO to drain
  while (LPSPI4_FSR & 0x0F) {
    Serial.printf("Draining... FSR: 0x%08X (TX: %d)\n", LPSPI4_FSR, LPSPI4_FSR & 0x0F);
    delay(1);
  }
  Serial.println("FIFO drained.");
  digitalWrite(OLED_CS_PIN, HIGH);
}


