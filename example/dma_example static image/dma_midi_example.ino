#include <Arduino.h>
#include <SPI.h>
#include "SSD1322.h"
#include "SSD1322_DMA.h"

// Pin definitions (adjust according to your setup)
#define OLED_CS_PIN    10
#define OLED_DC_PIN    9
#define OLED_RESET_PIN 8

// Forward declarations
void dmaCallback();
void testSpiMode();
void testDmaMode();

// Create instances
SSD1322 oled;
SSD1322_DMA oledDma;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Wait for Serial
  }
  Serial.println("\n\n----- SSD1322 DMA Test -----");
  
  // Setup SPI
  SPI.begin();
  
  // Configure OLED pins
  pinMode(OLED_CS_PIN, OUTPUT);
  pinMode(OLED_DC_PIN, OUTPUT);
  pinMode(OLED_RESET_PIN, OUTPUT);
  
  // Reset the display
  digitalWrite(OLED_RESET_PIN, LOW);
  delay(10);
  digitalWrite(OLED_RESET_PIN, HIGH);
  delay(100);
  
  // Initialize display
  oled.begin();
  oled.api.SSD1322_API_set_contrast(0x50); // Medium contrast
  
  // Initialize DMA
  oledDma.init(oled);
  oledDma.begin();
  oledDma.setDmaCompleteCallback(dmaCallback);
  
  // Short delay for everything to initialize
  delay(100);
  
  Serial.println("Setup complete, starting tests...");
}

void loop() {
  // Test regular SPI transfers first
  Serial.println("\n--- Standard SPI Test ---");
  testSpiMode();
  delay(2000);
  
  // Test DMA mode
  Serial.println("\n--- DMA Test ---");
  testDmaMode();
  delay(2000);
  
  // Try the interruptible draw method
  Serial.println("\n--- Interruptible DMA Test ---");
  bool completed = false;
  while (!completed) {
    completed = oledDma.draw_framebuffer_interruptible();
    delay(10);
  }
  delay(2000);
  
  Serial.println("All tests completed, waiting 10 seconds...");
  delay(10000);
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
  oled.api.SSD1322_API_clear();
  delay(50);
  
  // Set up window for drawing
  oled.api.SSD1322_API_set_window(0, (OLED_WIDTH / 2) - 1, 0, OLED_HEIGHT - 1);
  oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
  
  // Send test pattern through SPI
  digitalWrite(OLED_DC_PIN, HIGH); // Data mode
  digitalWrite(OLED_CS_PIN, LOW);  // Select display
  
  // Send test pattern
  uint32_t startTime = micros();
  
  for (int i = 0; i < 1024; i++) {
    // Create a gradient pattern
    SPI.transfer(i & 0xFF);
  }
  
  uint32_t endTime = micros();
  digitalWrite(OLED_CS_PIN, HIGH); // Deselect display
  
  Serial.printf("SPI transfer time: %u microseconds for 1024 bytes\n", endTime - startTime);
}

// Test with DMA transfers
void testDmaMode() {
  // Clear the display
  oled.api.SSD1322_API_clear();
  delay(50);
  
  // Fill buffer with test pattern
  for (int i = 0; i < DMABUFFER_SIZE && i < 1024; i++) {
    dma_buffer[i] = (i & 0xFF);
  }
  
  // Print first few bytes for debug
  Serial.print("DMA buffer preview: ");
  for (int i = 0; i < 16; i++) {
    Serial.printf("%02X ", dma_buffer[i]);
  }
  Serial.println();
  
  // Start DMA transfer
  uint32_t startTime = millis();
  oledDma.startDmaTransfer();
  
  // Wait for completion or timeout
  uint32_t timeout = 1000; // 1 second timeout
  while (!dmaDoneFlag && (millis() - startTime < timeout)) {
    // Wait for DMA to complete
    yield();
  }
  
  if (dmaDoneFlag) {
    Serial.println("DMA transfer completed");
  } else {
    Serial.println("DMA transfer timeout!");
  }
}