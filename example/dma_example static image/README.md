# SSD1322 DMA MIDI Example

This example demonstrates how to efficiently update an SSD1322 OLED display using DMA while simultaneously processing MIDI-like data streams.

## Features

- Uses DMA for non-blocking display updates
- Simulates MIDI data processing with 41μs timing
- Updates display animation with 20μs timing
- Shows performance statistics in real-time
- Visualizes MIDI note and velocity data

## Hardware Requirements

- Teensy 4.1 microcontroller
- SSD1322 OLED display (256x64 pixels, 4-bit grayscale)
- Connections:
  - SPI pins (MOSI, SCK)
  - CS (Pin 10)
  - DC (Pin 9)
  - RST (Pin 8)

## How to Run

1. Connect your SSD1322 OLED display to the Teensy 4.1
2. Open this project in PlatformIO
3. Build and upload to your Teensy
4. Open the serial monitor (115200 baud) to see performance statistics

## Performance Goals

- MIDI processing: ~41μs per message
- Display updates: ~20μs per frame
- Continuous DMA operation for smooth animation

## Understanding the Example

This example demonstrates how to:
1. Create an optimized framebuffer in DTCM memory
2. Set up DMA for continuous display updates
3. Perform time-critical operations (MIDI-like processing)
4. Update display content without blocking the CPU
5. Use memory barriers for thread-safe buffer updates

The animation shows:
- Horizontal lines representing MIDI notes
- Line width based on MIDI velocity
- Background pattern that animates over time
- Moving box animation
- Vertical bar showing MIDI message count

## Troubleshooting

If the display isn't updating or appears glitchy:
- Check your SPI connections
- Verify the CS, DC, and RST pin connections
- Make sure your display is compatible with the SSD1322 driver
- Check the serial output for any error messages

If DMA stops working:
- The code will automatically attempt to restart it
- Monitor the "DMA Active" status in the serial output 