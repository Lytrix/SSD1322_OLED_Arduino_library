# Minimal DMA SPI Example for Teensy 4.1

This project demonstrates a minimal DMA-to-SPI transfer using the Teensy 4.1 and the PJRC DMAChannel library, based on the approach from [mavriq-dev/Teensy-Documentation: DMA.md](https://github.com/mavriq-dev/Teensy-Documentation/blob/main/DMA.md).

- Uses LPSPI4 (pins 10-13 on Teensy 4.1).
- Transfers a buffer via DMA to SPI.
- Prints debug output on Serial.

## Wiring

- Connect a logic analyzer or loopback to MOSI (pin 11) and SCK (pin 13) to observe data.
- Or connect to an SPI device that can receive the data.

## Usage

1. Build and upload to Teensy 4.1.
2. Open Serial Monitor at 115200 baud.
3. Observe DMA transfer messages.