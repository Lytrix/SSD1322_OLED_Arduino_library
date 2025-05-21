#pragma once
#include <Arduino.h>

// DMA Register definitions for IMXRT1062 (Teensy 4.1)
#define DMA_TCD_SADDR(n)             (*(volatile uint32_t*)(0x400E9000 + 0x1000 * n))
#define DMA_TCD_DADDR(n)             (*(volatile uint32_t*)(0x400E9008 + 0x1000 * n))
#define DMA_TCD_ATTR(n)              (*(volatile uint16_t*)(0x400E9004 + 0x1000 * n))
#define DMA_TCD_NBYTES_MLNO(n)       (*(volatile uint32_t*)(0x400E9010 + 0x1000 * n))
#define DMA_TCD_CITER_ELINKNO(n)     (*(volatile uint16_t*)(0x400E9016 + 0x1000 * n))
#define DMA_TCD_BITER_ELINKNO(n)     (*(volatile uint16_t*)(0x400E901E + 0x1000 * n))
#define DMA_TCD_CSR(n)               (*(volatile uint16_t*)(0x400E901C + 0x1000 * n))
#define DMAMUX_CHCFG(n)              (*(volatile uint32_t*)(0x400EC000 + 4 * n))
#define DMA_SERQ                     (*(volatile uint8_t *)0x400E100D)
#define DMA_CINT                     (*(volatile uint8_t *)0x400E100C)

// DMA Attribute register bit definitions
#define DMA_TCD_ATTR_SSIZE(n)        (((n) & 0x7) << 0)
#define DMA_TCD_ATTR_DSIZE(n)        (((n) & 0x7) << 3)

// DMA CSR bits
#define DMA_TCD_CSR_INTMAJOR        0x0002

// CCM register for clock gating
#define CCM_CCGR_ON                  0x03
#define CCM_CCGR5_DMA(n)            ((uint32_t)(n) << 6)

class LowLevelDMATransfer {
private:
    static constexpr uint8_t DMA_CHANNEL = 0;
    DMAMEM static uint8_t _source[256] __attribute__((aligned(32))); // normally uint32_t
    DMAMEM static uint8_t _dest[256] __attribute__((aligned(32)));

public:
    void begin() {
        // Enable DMA clock
        CCM_CCGR5 |= CCM_CCGR5_DMA(CCM_CCGR_ON);
        // Configure DMA MUX
        DMAMUX_CHCFG(DMA_CHANNEL) = 0;
        // Setup TCD (Transfer Control Descriptor)
        DMA_TCD_SADDR(DMA_CHANNEL) = (uint32_t)_source;
        DMA_TCD_DADDR(DMA_CHANNEL) = (uint32_t)_dest;
        DMA_TCD_ATTR(DMA_CHANNEL) = DMA_TCD_ATTR_SSIZE(0) |  // 8/16/32-bit source
                                   DMA_TCD_ATTR_DSIZE(0);    // 8/16/32-bit dest (0/1/2)
        DMA_TCD_NBYTES_MLNO(DMA_CHANNEL) = 1;  // 1-4 bytes per transfer
        DMA_TCD_CITER_ELINKNO(DMA_CHANNEL) = sizeof(_source) / 1; //(1/2/4)
        DMA_TCD_BITER_ELINKNO(DMA_CHANNEL) = sizeof(_source) / 1; //(1/2/4)
        // Enable interrupt
        DMA_TCD_CSR(DMA_CHANNEL) = DMA_TCD_CSR_INTMAJOR;
        attachInterruptVector(static_cast<IRQ_NUMBER_t>(IRQ_DMA_CH0 + DMA_CHANNEL), dmaISR);
        NVIC_ENABLE_IRQ(IRQ_DMA_CH0 + DMA_CHANNEL);
        // Start transfer
        DMA_SERQ = DMA_CHANNEL;
    }

    // Setters
    static void setSource(const uint32_t* src, size_t count) {
        memcpy(_source, src, count * sizeof(uint32_t));
    }
    static void setDest(const uint32_t* dst, size_t count) {
        memcpy(_dest, dst, count * sizeof(uint32_t));
    }
    // Getters
    static uint32_t* getSource() { return _source; }
    static uint32_t* getDest() { return _dest; }

private:
    static void dmaISR() {
        // Clear interrupt flag
        DMA_CINT = DMA_CHANNEL;
        // Handle completion
        digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
    }
};

