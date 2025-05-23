#ifndef SSD1322_DMA_H
#define SSD1322_DMA_H

#include "MasterBase.h"
#include <DMAChannel.h>
#include <SPI.h>

class SSD1322_DMA : public MasterBase {
public:
    SSD1322_DMA(SPIClass& spi, uint8_t spi_index);
    ~SSD1322_DMA();

    bool begin(uint8_t cs, const SPISettings& settings, bool active_low = true);

    // All other methods are inherited from MasterBase
    // (queue, remained, etc.)

protected:
    // Implement required virtuals for DMA setup
    virtual DMAChannel* dmarx() override;
    virtual DMAChannel* dmatx() override;
    virtual bool initDmaTx() override;
    virtual bool initDmaRx() override;
    virtual void destroy() override;
    virtual void initTransaction() override;
    virtual void clearTransaction() override;

private:
    SPIClass& _spi;
    uint8_t _spi_index;
    DMAChannel* _rx;
    DMAChannel* _tx;
}; extern SSD1322_DMA dmaSpi;

#endif // SSD1322_DMA_H 