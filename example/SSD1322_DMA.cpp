#include "SSD1322_DMA.h"
#include "imxrt.h" // For register access, if needed

SSD1322_DMA::SSD1322_DMA(SPIClass& spi, uint8_t spi_index)
    : _spi(spi), _spi_index(spi_index), _rx(nullptr), _tx(nullptr) {}

SSD1322_DMA::~SSD1322_DMA() {
    destroy();
}

bool SSD1322_DMA::begin(uint8_t cs, const SPISettings& settings, bool active_low) {
    return MasterBase::begin(_spi, cs, settings, active_low);
}

DMAChannel* SSD1322_DMA::dmarx() {
    if (!_rx) _rx = new DMAChannel();
    return _rx;
}
DMAChannel* SSD1322_DMA::dmatx() {
    if (!_tx) _tx = new DMAChannel();
    return _tx;
}

bool SSD1322_DMA::initDmaTx() {
    if (dmatx() == nullptr) return false;
    dmatx()->disable();
    switch (_spi_index) {
        case 0: dmatx()->destination((volatile uint8_t &)IMXRT_LPSPI4_S.TDR); dmatx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX); break;
        case 1: dmatx()->destination((volatile uint8_t &)IMXRT_LPSPI3_S.TDR); dmatx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI3_TX); break;
        case 2: dmatx()->destination((volatile uint8_t &)IMXRT_LPSPI1_S.TDR); dmatx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI1_TX); break;
        default: return false;
    }
    dmatx()->disableOnCompletion();
    return !dmatx()->error();
}

bool SSD1322_DMA::initDmaRx() {
    if (dmarx() == nullptr) return false;
    dmarx()->disable();
    switch (_spi_index) {
        case 0: dmarx()->source((volatile uint8_t &)IMXRT_LPSPI4_S.RDR); dmarx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_RX); break;
        case 1: dmarx()->source((volatile uint8_t &)IMXRT_LPSPI3_S.RDR); dmarx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI3_RX); break;
        case 2: dmarx()->source((volatile uint8_t &)IMXRT_LPSPI1_S.RDR); dmarx()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI1_RX); break;
        default: return false;
    }
    dmarx()->disableOnCompletion();
    // Interrupts can be attached here if needed
    return !dmarx()->error();
}

void SSD1322_DMA::destroy() {
    if (_rx) { delete _rx; _rx = nullptr; }
    if (_tx) { delete _tx; _tx = nullptr; }
}

void SSD1322_DMA::initTransaction() {
    // Set up the correct SPI registers for 8-bit mode, etc.
    switch (_spi_index) {
        case 0:
            IMXRT_LPSPI4_S.TCR = (IMXRT_LPSPI4_S.TCR & ~(LPSPI_TCR_FRAMESZ(31))) | LPSPI_TCR_FRAMESZ(7);
            IMXRT_LPSPI4_S.FCR = 0;
            IMXRT_LPSPI4_S.DER = LPSPI_DER_TDDE | LPSPI_DER_RDDE;
            IMXRT_LPSPI4_S.SR = 0x3f00;
            break;
        case 1:
            IMXRT_LPSPI3_S.TCR = (IMXRT_LPSPI3_S.TCR & ~(LPSPI_TCR_FRAMESZ(31))) | LPSPI_TCR_FRAMESZ(7);
            IMXRT_LPSPI3_S.FCR = 0;
            IMXRT_LPSPI3_S.DER = LPSPI_DER_TDDE | LPSPI_DER_RDDE;
            IMXRT_LPSPI3_S.SR = 0x3f00;
            break;
        case 2:
            IMXRT_LPSPI1_S.TCR = (IMXRT_LPSPI1_S.TCR & ~(LPSPI_TCR_FRAMESZ(31))) | LPSPI_TCR_FRAMESZ(7);
            IMXRT_LPSPI1_S.FCR = 0;
            IMXRT_LPSPI1_S.DER = LPSPI_DER_TDDE | LPSPI_DER_RDDE;
            IMXRT_LPSPI1_S.SR = 0x3f00;
            break;
    }
}

void SSD1322_DMA::clearTransaction() {
    switch (_spi_index) {
        case 0:
            IMXRT_LPSPI4_S.FCR = LPSPI_FCR_TXWATER(15);
            IMXRT_LPSPI4_S.DER = 0;
            IMXRT_LPSPI4_S.CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;
            IMXRT_LPSPI4_S.SR = 0x3f00;
            break;
        case 1:
            IMXRT_LPSPI3_S.FCR = LPSPI_FCR_TXWATER(15);
            IMXRT_LPSPI3_S.DER = 0;
            IMXRT_LPSPI3_S.CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;
            IMXRT_LPSPI3_S.SR = 0x3f00;
            break;
        case 2:
            IMXRT_LPSPI1_S.FCR = LPSPI_FCR_TXWATER(15);
            IMXRT_LPSPI1_S.DER = 0;
            IMXRT_LPSPI1_S.CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;
            IMXRT_LPSPI1_S.SR = 0x3f00;
            break;
    }
} 