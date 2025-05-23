#include <Arduino.h>
#include <SPI.h>
#include "SSD1322.h"
#include <TsyDMASPI.h>

// Configuration constants
#define DMABUFFER_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 2)  // 4bpp packed pixels

DMAMEM uint8_t dma_buffer[DMABUFFER_SIZE] __attribute__((aligned(32)));
DMAMEM uint8_t pixel_buffer[DMABUFFER_SIZE] __attribute__((aligned(32)));

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
void updateDisplayFrame();

// Create instances
SSD1322 oled(OLED_CS_PIN, OLED_DC_PIN, OLED_HEIGHT, OLED_WIDTH, OLED_RESET_PIN);

// Add this at the top, after includes and before setup():
// uint8_t dma_buffer[(OLED_WIDTH / 2) * OLED_HEIGHT];

#define DISPLAY_FRAME_INTERVAL_US 33333  // 30fps = 33.333ms
#define MIDI_UPDATE_INTERVAL_US 20       // 20us per MIDI event

volatile uint32_t frameCount = 0;
volatile uint32_t midiCount = 0;
volatile int32_t maxMidiJitter = 0;
volatile int32_t maxDisplayJitter = 0;

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
  TsyDMASPI0.begin(OLED_CS_PIN, SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  // Short delay for everything to initialize
  delay(100);
  
  Serial.println("Setup complete, starting tests...");
}

void loop() {
  uint32_t now = micros();

  // MIDI update section (every 20us)
  static uint32_t lastMidi = 0;
  int32_t midiJitter = (int32_t)(now - lastMidi) - MIDI_UPDATE_INTERVAL_US;
  if (midiJitter > maxMidiJitter) maxMidiJitter = midiJitter;
  if ((now - lastMidi) >= MIDI_UPDATE_INTERVAL_US) {
    lastMidi = now;
    midiCount++;
    // Simulate MIDI processing (very short)
    // ... (put your MIDI code here)
  }

  // Display update section (every 33.3ms)
  static uint32_t lastDisplay = 0;
  static uint32_t fillTimeSum = 0;
  static uint32_t dmaTimeSum = 0;
  static uint32_t frameCounter = 0;
  if ((now - lastDisplay) >= DISPLAY_FRAME_INTERVAL_US) {
    lastDisplay = now;
    frameCount++;

    // --- Timing for buffer fill and DMA transfer ---
    uint32_t fillStart = micros();

    // Fill buffer with animation (same as in updateDisplayFrame)
    static int8_t bg_brightness = 0; // 0-15
    static int8_t bg_dir = 1; // 1 for up, -1 for down
    static uint32_t lastBgUpdate = 0;
    uint32_t now_ms = millis();
    if (now_ms - lastBgUpdate >= 1000) {
      lastBgUpdate = now_ms;
      bg_brightness += bg_dir;
      if (bg_brightness >= 15) {
        bg_brightness = 15;
        bg_dir = -1;
      } else if (bg_brightness <= 0) {
        bg_brightness = 0;
        bg_dir = 1;
      }
    }
    uint8_t bar = 0xF; // max brightness for 4bpp
    uint8_t bar_y = frameCount % OLED_HEIGHT;
    uint8_t line_x = frameCount % OLED_WIDTH;
    for (uint32_t y = 0; y < OLED_HEIGHT; ++y) {
      for (uint32_t x = 0; x < OLED_WIDTH; x += 2) {
        uint8_t left = (x < (OLED_WIDTH/2)) ? bg_brightness : 0x1;
        uint8_t right = ((x+1) < (OLED_WIDTH/2)) ? bg_brightness : 0x1;
        if (y == bar_y) { left = bar; right = bar; }
        if (x == line_x) left = bar;
        if ((x+1) == line_x) right = bar;
        pixel_buffer[(y * (OLED_WIDTH / 2)) + (x / 2)] = (left << 4) | (right & 0x0F);
      }
    }

   
    uint32_t fillEnd = micros();
    fillTimeSum += (fillEnd - fillStart);
  
    memcpy(dma_buffer, pixel_buffer, DMABUFFER_SIZE);  
    // DMA transfer timing
    
    oled.api.SSD1322_API_set_window(0, 63, 0, 63);
    oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
    digitalWrite(OLED_DC_PIN, HIGH);
    digitalWrite(OLED_CS_PIN, LOW);
    uint32_t dmaStart = micros();
      arm_dcache_flush((void*)dma_buffer, DMABUFFER_SIZE);
    TsyDMASPI0.queue(dma_buffer, DMABUFFER_SIZE);
    while (TsyDMASPI0.remained() > 0) {}
    digitalWrite(OLED_CS_PIN, HIGH);
    uint32_t dmaEnd = micros();
    dmaTimeSum += (dmaEnd - dmaStart);

    frameCounter++;
    if (frameCounter >= 30) {
      Serial.printf("Avg buffer fill: %lu us | Avg DMA transfer: %lu us\n", fillTimeSum / frameCounter, dmaTimeSum / frameCounter);
      fillTimeSum = 0;
      dmaTimeSum = 0;
      frameCounter = 0;
    }
  }
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
  // Animation: moving vertical bar
  uint8_t bg = 0x11; // dim background
  uint8_t bar = 0xFF; // bright bar
  uint8_t bar_x = frameCount % OLED_WIDTH;

  uint32_t fillStart = micros();
  for (uint32_t y = 0; y < OLED_HEIGHT; ++y) {
    for (uint32_t x = 0; x < OLED_WIDTH; x += 2) {
      // Each byte holds two pixels (4bpp)
      uint8_t left = (x == bar_x) ? bar : bg;
      uint8_t right = ((x+1) == bar_x) ? bar : bg;
      dma_buffer[(y * (OLED_WIDTH / 2)) + (x / 2)] = (left << 4) | (right & 0x0F);
    }
  }
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
  TsyDMASPI0.queue(dma_buffer, DMABUFFER_SIZE);
  uint32_t cpuEnd = micros();
  Serial.printf("CPU busy time for DMA setup: %u microseconds\n", cpuEnd - cpuStart);

  // Measure total elapsed time for DMA transfer (CPU is free during this time)
  uint32_t dmaStart = micros();
  while (TsyDMASPI0.remained() > 0) {
    // Optionally do other work here
    // yield(); // Uncomment if you want to yield
  }
  uint32_t dmaEnd = micros();
  digitalWrite(OLED_CS_PIN, HIGH);
  Serial.printf("Total elapsed time for DMA transfer (CPU free): %u microseconds\n", dmaEnd - dmaStart);
}

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

void updateDisplayFrame() {
  static int8_t bg_brightness = 0; // 0-15
  static int8_t bg_dir = 1; // 1 for up, -1 for down
  static uint32_t lastBgUpdate = 0;
  uint32_t now = millis();
  if (now - lastBgUpdate >= 1000) {
    lastBgUpdate = now;
    bg_brightness += bg_dir;
    if (bg_brightness >= 15) {
      bg_brightness = 15;
      bg_dir = -1;
    } else if (bg_brightness <= 0) {
      bg_brightness = 0;
      bg_dir = 1;
    }
  }

  uint8_t bar = 0xF; // max brightness for 4bpp
  uint8_t bar_y = frameCount % OLED_HEIGHT;
  uint8_t line_x = frameCount % OLED_WIDTH;

  for (uint32_t y = 0; y < OLED_HEIGHT; ++y) {
    for (uint32_t x = 0; x < OLED_WIDTH; x += 2) {
      // Background: left 0-50% of screen
      uint8_t left = (x < (OLED_WIDTH/2)) ? bg_brightness : 0x1;
      uint8_t right = ((x+1) < (OLED_WIDTH/2)) ? bg_brightness : 0x1;
      // Horizontal bar
      if (y == bar_y) { left = bar; right = bar; }
      // Vertical line
      if (x == line_x) left = bar;
      if ((x+1) == line_x) right = bar;
      dma_buffer[(y * (OLED_WIDTH / 2)) + (x / 2)] = (left << 4) | (right & 0x0F);
    }
  }
  arm_dcache_flush((void*)dma_buffer, DMABUFFER_SIZE);
  oled.api.SSD1322_API_set_window(0, 63, 0, 63);
  oled.api.SSD1322_API_command(SSD1322_WRITE_RAM);
  digitalWrite(OLED_DC_PIN, HIGH);
  digitalWrite(OLED_CS_PIN, LOW);
  TsyDMASPI0.queue(dma_buffer, DMABUFFER_SIZE);
  while (TsyDMASPI0.remained() > 0) {}
  digitalWrite(OLED_CS_PIN, HIGH);
}