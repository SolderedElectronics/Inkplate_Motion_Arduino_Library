// Include class header file.
#include "microSD.h"

#ifdef LIBRARY_FEATUTE_MICROSD_ENABLED

MicroSD::MicroSD()
{
}

MicroSD::~MicroSD()
{
    delete stm32SpiCfg;
}

void MicroSD::begin(SPIClass *_sdSpi, int _cs, int _speedMhz)
{
    stm32SpiCfg = new SdSpiConfig((SdCsPin_t)_cs, SHARED_SPI, SD_SCK_MHZ(_speedMhz), _sdSpi);
}

int MicroSD::sdCardInit()
{
    int sdCardOk = sd.begin(*stm32SpiCfg);
    return sdCardOk;
}
#endif