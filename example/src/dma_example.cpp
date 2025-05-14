// SSD1322 OLED DMA Example with MIDI processing
// For Teensy 4.x - demonstrates frame buffer DMA interruptible updates
// This example shows how to use DMA to update the display in small chunks
// to free up CPU for time-sensitive operations like MIDI processing

#include <Arduino.h>
#include <SPI.h>
#include "SSD1322.h"

// SPI Pins
#define OLED_CLOCK 13
#define OLED_DATA 11
#define OLED_CS 10
#define OLED_DC 9
#define OLED_RESET 8 

// Display size
#define OLED_WIDTH 256
#define OLED_HEIGHT 64

// Buffer and display instance
uint8_t frameBuffer[OLED_WIDTH * OLED_HEIGHT / 2];
SSD1322 display(OLED_CS, OLED_DC, OLED_HEIGHT, OLED_WIDTH, OLED_RESET);

// Animation variables
unsigned long lastFrameTime = 0;
const unsigned long FRAME_INTERVAL = 33; // ~30fps
int animationX = 0;
bool frameUpdateInProgress = false;

// MIDI processing simulation
unsigned long lastMidiTime = 0;
const unsigned long MIDI_INTERVAL = 2; // 2ms - simulate frequent MIDI events
unsigned long midiEventsProcessed = 0;
unsigned long displayUpdatesCompleted = 0;

// DMA mode selection
bool useDmaMode = true;
bool useDmaCircular = false;  // Use circular DMA (continuous update) or interruptible (chunked update)

// Font selection
const GFXfont *currentFont = nullptr;

// Clear display buffer and send to display
void clearDisplayBuffer() {
  display.gfx.fill_buffer(frameBuffer, 0);
  display.gfx.send_buffer_to_OLED(frameBuffer, 0, 0);
}

// Function to draw an animated scene to the buffer
void updateAnimationBuffer() {
  // Clear the buffer first
  display.gfx.fill_buffer(frameBuffer, 0);
  
  // Draw a moving box
  int boxX = (animationX % (OLED_WIDTH * 2));
  if (boxX > OLED_WIDTH) boxX = OLED_WIDTH * 2 - boxX; // Bounce back
  
  display.gfx.draw_rect_filled(frameBuffer, boxX, 10, boxX + 40, 30, 15);
  
  // Draw some text
  display.gfx.draw_text(frameBuffer, "DMA OLED Demo", 80, 5, 10);
  
  // Draw MIDI event counter
  char counterText[32];
  sprintf(counterText, "MIDI: %lu", midiEventsProcessed);
  display.gfx.draw_text(frameBuffer, counterText, 10, 40, 12);
  
  // Draw display update counter
  sprintf(counterText, "Updates: %lu", displayUpdatesCompleted);
  display.gfx.draw_text(frameBuffer, counterText, 10, 55, 12);
  
  // Advance animation
  animationX += 2;
}

// Simulated MIDI processing function - this would normally handle real MIDI events
void processMidiEvents() {
  // In a real application, you'd check for and process MIDI messages here
  // This is just a simulation that increments a counter
  midiEventsProcessed++;
  
  // Simulate some processing time
  delayMicroseconds(100);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for serial or timeout
  
  Serial.println("SSD1322 OLED DMA Example");
  
  // Set up SPI pins
  pinMode(OLED_CS, OUTPUT);
  pinMode(OLED_DC, OUTPUT);
  if (OLED_RESET != -1) {
    pinMode(OLED_RESET, OUTPUT);
  }
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize display
  display.api.SSD1322_API_init();
  
  // Set buffer size
  display.gfx.set_buffer_size(OLED_WIDTH, OLED_HEIGHT);
  
  // Clear display
  clearDisplayBuffer();
  
  // Initial display message
  display.gfx.draw_text(frameBuffer, "DMA OLED Demo", 80, 20, 15);
  display.gfx.draw_text(frameBuffer, "Starting...", 90, 35, 15);
  display.gfx.send_buffer_to_OLED(frameBuffer, 0, 0);
  delay(1000);
  
  // Initialize DMA if using it
  #ifdef __IMXRT1062__
  if (useDmaMode) {
    if (useDmaCircular) {
      Serial.println("Initializing DMA circular buffer mode");
      display.beginDMA();
    } else {
      Serial.println("Using DMA interruptible mode");
    }
  }
  #endif
  
  Serial.println("Setup complete");
}

void loop() {
  // Process MIDI events at high frequency
  if (micros() - lastMidiTime >= MIDI_INTERVAL * 1000) {
    lastMidiTime = micros();
    processMidiEvents();
  }
  
  // Update the animation at a lower frequency
  if (millis() - lastFrameTime >= FRAME_INTERVAL) {
    lastFrameTime = millis();
    
    // Update the animation in the buffer
    updateAnimationBuffer();
    
    // Start a new frame update if not already in progress
    if (!frameUpdateInProgress) {
      frameUpdateInProgress = true;
    }
  }
  
  // If a frame update is in progress, push the next chunk
  if (frameUpdateInProgress) {
    #ifdef __IMXRT1062__
    if (useDmaMode && !useDmaCircular) {
      // Use interruptible frame buffer updates on Teensy 4.x
      bool frameComplete = display.drawFrameBufferInterruptible(frameBuffer);
      if (frameComplete) {
        frameUpdateInProgress = false;
        displayUpdatesCompleted++;
      }
    } else {
      // if not using DMA, use standard update method
      display.gfx.send_buffer_to_OLED(frameBuffer, 0, 0);
      frameUpdateInProgress = false;
      displayUpdatesCompleted++;
    }
    #else
    // On other platforms, use standard update method
    display.gfx.send_buffer_to_OLED(frameBuffer, 0, 0);
    frameUpdateInProgress = false;
    displayUpdatesCompleted++;
    #endif
  }
} 