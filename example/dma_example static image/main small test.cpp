#include <Arduino.h>
#include <imxrt.h>
#include <DMAChannel.h>
#include <SPI.h>

#define CS_PIN 10
#define DC_PIN 9
#define DEBUG_LED 13

DMAMEM uint8_t test_buffer[8] __attribute__((aligned(32))) = {0xAA, 0x55, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44};
DMAChannel *dma = nullptr;
volatile bool dmaDone = false;

void dma_isr() {
    if (dma) dma->clearInterrupt();
    dmaDone = true;
    digitalWriteFast(DEBUG_LED, !digitalReadFast(DEBUG_LED));
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Minimal LPSPI4 DMA Test ---");

    pinMode(CS_PIN, OUTPUT);
    pinMode(DC_PIN, OUTPUT);
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    digitalWrite(DC_PIN, HIGH);
    digitalWrite(DEBUG_LED, LOW);

    // SPI pin muxing (Teensy 4.1 default MOSI: 11, SCK: 13)
    SPI.begin();
    delay(10);

    // Configure SPI registers
    LPSPI4_CR &= ~LPSPI_CR_MEN;
    LPSPI4_CR |= LPSPI_CR_RTF | LPSPI_CR_RRF;
    LPSPI4_FCR = 0;
    LPSPI4_CR |= LPSPI_CR_MEN;
    LPSPI4_TCR = (LPSPI_TCR_FRAMESZ(7) | (0 << 16)); // 8-bit, PCS=0
    LPSPI4_CFGR1 = 0x00000009;
    LPSPI4_SR = 0x3F00;
    LPSPI4_DER = LPSPI_DER_TDDE;

    // Flush cache for DMA buffer
    arm_dcache_flush((void*)test_buffer, sizeof(test_buffer));

    // Set CS and DC
    digitalWrite(DC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CS_PIN, LOW);
    delayMicroseconds(10);

    // Prime FIFO
    for (int i = 0; i < 4; i++) {
        LPSPI4_TDR = test_buffer[i];
        delayMicroseconds(1);
    }
    delayMicroseconds(50);

    // Set up DMA
    dma = new DMAChannel();
    dma->disable();
    dma->TCD->SADDR = test_buffer;
    dma->TCD->SOFF = 1;
    dma->TCD->ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);
    dma->TCD->NBYTES_MLNO = 1;
    dma->TCD->SLAST = -((int32_t)sizeof(test_buffer));
    dma->TCD->DADDR = (volatile void *)&LPSPI4_TDR;
    dma->TCD->DOFF = 0;
    dma->TCD->CITER_ELINKNO = sizeof(test_buffer);
    dma->TCD->DLASTSGA = 0;
    dma->TCD->BITER_ELINKNO = sizeof(test_buffer);
    dma->TCD->CSR = DMA_TCD_CSR_INTMAJOR;
    dma->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX);
    dma->attachInterrupt(dma_isr);

    dmaDone = false;
    dma->enable();
    Serial.println("DMA started");
}

void loop() {
    if (dmaDone) {
        Serial.println("DMA complete!");
        digitalWrite(CS_PIN, HIGH);
        while (1) {
            digitalWrite(DEBUG_LED, HIGH);
            delay(100);
            digitalWrite(DEBUG_LED, LOW);
            delay(100);
        }
    }
} 