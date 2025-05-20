#include <Arduino.h>
#include "SSD1322.h"
#include "SSD1322_DMA.h"
#include "SSD1322_Config.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include <cstdint>

// Create OLED display instance
SSD1322 display(config.OLED_CS_PIN, config.OLED_DC_PIN, config.OLED_RESET_PIN, config.OLED_HEIGHT, config.OLED_WIDTH);

// MIDI timing emulation
const uint32_t MIDI_INTERVAL_US = 41;  // Approx MIDI message timing
volatile uint32_t last_midi_time = 0;
volatile uint32_t midi_message_count = 0;
volatile uint8_t midi_note = 60;  // Middle C
volatile uint8_t midi_velocity = 64;

// Display timing
const uint32_t DISPLAY_UPDATE_INTERVAL_US = 20;  // Fast display updates
volatile uint32_t last_display_update_time = 0;
volatile uint32_t frame_count = 0;

// Animation state
uint8_t animation_phase = 0;
const uint8_t animation_speed = 1;

// For performance tracking
uint32_t last_stats_time = 0;
uint32_t display_update_time_us = 0;
uint32_t midi_process_time_us = 0;

// Callback function for when DMA frame is complete
void frameCompleteCB() {
  frame_count++;
}

// Function to simulate MIDI message processing
void processMidiMessage() {
  // Start timing
  uint32_t start_time = micros();
  
  // Simulate MIDI processing load (about 40us worth of work)
  uint8_t random_note = random(36, 84);  // Random note between C2 and C6
  uint8_t random_velocity = random(1, 127);
  
  // Only update if it has changed (to avoid excessive variables updates)
  if (random_note != midi_note || random_velocity != midi_velocity) {
    midi_note = random_note;
    midi_velocity = random_velocity;
    midi_message_count++;
  }
  
  // Burn some CPU cycles to simulate real MIDI processing
  // This is a way to create a somewhat predictable processing load
  uint32_t calc = 0;
  for (int i = 0; i < 1000; i++) {
    calc += (midi_note * midi_velocity) & 0xFF;
  }
  
  // Update timing metrics
  midi_process_time_us = micros() - start_time;
}

// Update display frame buffer with current animation
void updateFrameBuffer() {
    // Start timing
    uint32_t start_time = micros();
    //Serial.println("Updating frame buffer...");

    // Draw into the GFX buffer (draw buffer)
    //Serial.println("Getting frame buffer...");
    uint8_t* draw_fb = display.gfx.get_frame_buffer();
    //Serial.println("Frame buffer obtained");

    // Clear the buffer first
    //Serial.println("Clearing buffer...");
    display.gfx.fill_buffer(draw_fb, 0);
    //Serial.println("Buffer cleared");

    // Draw animation based on current phase
    animation_phase += animation_speed;
    //Serial.println("Drawing animation...");

    // Define safe bounds for drawing operations
    const int max_height = std::min(OLED_HEIGHT, 64); // Ensure we don't exceed reasonable bounds
    const int max_width = std::min(OLED_WIDTH, 256);  // Ensure we don't exceed reasonable bounds

    // Debug - horizontal line
    //Serial.println("  1. Drawing horizontal line...");
    int line_y = (midi_note % max_height);
    int line_width = (midi_velocity * max_width) / 127;
    if (line_width > 0 && line_y >= 0 && line_y < max_height) {
        display.gfx.draw_hline(draw_fb, line_y, 0, std::min(line_width - 1, max_width - 1), 15);
    }
  
    // Debug - background pattern
    //Serial.println("  2. Drawing background pattern...");
    for (int y = 0; y < max_height; y++) {
        if (y % 10 == 0) {
            //Serial.printf("    Background row %d/%d\n", y, max_height);
        }
        for (int x = 0; x < max_width; x += 8) {
            display.gfx.draw_pixel(draw_fb, x, y, 5);
        }
    }

    // Debug - moving box
    //Serial.println("  3. Drawing moving box...");
    int box_x = (animation_phase * 2) % max_width;
    int box_y = (animation_phase / 2) % max_height;
    int box_size = 5;
    if (box_x >= 0 && box_y >= 0) {
        display.gfx.draw_rect_filled(
            draw_fb,
            box_x,
            box_y,
            std::min(box_x + box_size - 1, max_width - 1),
            std::min(box_y + box_size - 1, max_height - 1),
            15
        );
    }

    // Debug - MIDI message count
    //Serial.println("  4. Drawing MIDI message viz...");
    int message_height = (midi_message_count % max_height);
    if (message_height > 0 && max_width > 0) {
        display.gfx.draw_vline(
            draw_fb,
            max_width - 1,
            max_height - message_height,
            max_height - 1,
            15
        );
    }

    // Instead of copying to DMA buffer and triggering DMA here,
    // call the DMA driver's draw_framebuffer_interruptible function
    if (!display.dmaDriver.isRunning()) {
        display.dmaDriver.draw_framebuffer_interruptible(draw_fb);
    }
    
    display_update_time_us = micros() - start_time;
  if (!display.dmaDriver.isRunning()) {
    //Serial.printf("Frame buffer update completed in %d us\n", display_update_time_us);
  }
}

void createTestPattern(SSD1322_GFX& gfx) {
    gfx.clear_display_buffer();
    uint8_t* pixelBuffer = gfx.get_frame_buffer();
    // after your drawing calls:
    gfx.fill_buffer(pixelBuffer, 0); // Clear to black

    // Draw a white border
    gfx.draw_rect(pixelBuffer, 0, 0, OLED_WIDTH-1, OLED_HEIGHT-1, 15);

    // Draw diagonal lines
    gfx.draw_line(pixelBuffer, 0, 0, OLED_WIDTH-1, OLED_HEIGHT-1, 8);
    gfx.draw_line(pixelBuffer, 0, OLED_HEIGHT-1, OLED_WIDTH-1, 0, 8);

    // Draw a filled rectangle in the center
    int rect_w = OLED_WIDTH / 4;
    int rect_h = OLED_HEIGHT / 4;
    gfx.draw_rect_filled(pixelBuffer, OLED_WIDTH/2 - rect_w/2, OLED_HEIGHT/2 - rect_h/2,
                              OLED_WIDTH/2 + rect_w/2, OLED_HEIGHT/2 + rect_h/2, 4);

    // Draw a circle in the center
    gfx.draw_circle(pixelBuffer, OLED_WIDTH/2, OLED_HEIGHT/2, OLED_HEIGHT/4, 12);

    // Draw some text (if font is set)
    gfx.select_font(&FreeSansBold12pt7b);
    gfx.draw_text(pixelBuffer, "<TEST>", 10, OLED_HEIGHT/2, 15);
}

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(1000);  // Give some time for Serial connection
  

  Serial.println("\n\nSSD1322 DMA MIDI Example");
  Serial.println("=======================");
  
  Serial.println("Initializing SPI...");
  SPI.begin();

  Serial.println("Initializing display...");
  display.api.SSD1322_API_init();
  Serial.println("Display initialized");

  Serial.println("Create SPI Test pattern...");
  createTestPattern(display.gfx);
  
  // Send to display using direct SPI first to verify connectivity
  Serial.println("Send to display via SPI...");
  display.gfx.send_buffer_to_OLED(display.gfx.get_frame_buffer(), 0, 0); 
  Serial.println("SPI test pattern sent");
  delay(1000); // Let user see the test pattern
  Serial.println("Delay");
  // Initialize the DMA controller
  Serial.println("Initializing DMA...");
  //#ifdef __IMXRT1062__
  display.beginDMA();

  Serial.printf("Framebuffer address: %p\n", display.dmaDriver.getDMABuffer());
  Serial.printf("Alignment: %d\n", ((uintptr_t)display.dmaDriver.getDMABuffer()) % 32);
 // Serial.printf("DMA status: %s\n", display.isDMARunning() ? "RUNNING" : "NOT RUNNING");
 // #else
  //Serial.println("WARNING: DMA not supported on this platform");
  //#endif
  
  // Initialize random seed
  randomSeed(analogRead(A0));
  
  // Initialize timing variables
  last_midi_time = micros();
  last_display_update_time = micros();
  last_stats_time = millis();
  Serial.println("Setup complete!");
  delay(1000);
}

uint32_t lastFrameTime = 0;
const uint32_t FRAME_INTERVAL_MS = 16; // ~60Hz

void loop() {
    uint32_t now = millis();

    // Only update if enough time has passed and DMA is not running
    if ((now - lastFrameTime) >= FRAME_INTERVAL_MS && !display.dmaDriver.isRunning()) {
        lastFrameTime = now;
        updateFrameBuffer(); // This will call draw_framebuffer_interruptible()
    }

    // Call draw_framebuffer_interruptible() repeatedly to advance DMA state machine
    // (if you want to decouple drawing and DMA, you can call it unconditionally)
    display.dmaDriver.draw_framebuffer_interruptible(display.gfx.get_frame_buffer());

    uint32_t current_time_us = micros();

    // Process MIDI at ~41us intervals
    if (current_time_us - last_midi_time >= MIDI_INTERVAL_US) {
        last_midi_time = current_time_us;
        processMidiMessage();
    }
} 