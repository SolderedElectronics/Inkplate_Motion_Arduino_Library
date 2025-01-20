// Header guard.
#ifndef __INKPLATE_MICROSD_H__
#define __INKPLATE_MICROSD_H__

// Featue select guard.
#ifdef LIBRARY_FEATUTE_MICROSD_ENABLED

// Include main Arduino Header file.
#include "Arduino.h"

// Get SPI pins.
#include "../../boards/Inkplate6Motion/pins.h"

class MicroSD
{
  public:
    // Constructor.
    MicroSD();
    ~MicroSD();
    void begin(SPIClass *_sdSpi, int _cs, int _speedMhz);
    int sdCardInit();
    SdFat sd;

  private:
    // Settings for the dedicated microSD card SPI port.
    SdSpiConfig *stm32SpiCfg = nullptr;
};

#endif

#endif