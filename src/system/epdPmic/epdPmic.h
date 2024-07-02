#ifndef __EPDPMIC_H__
#define __EPDPMIC_H__

// Include Arduino Header file.
#include "Arduino.h"

// Include Arduino I2C library
#include <Wire.h>

// Include all TPS651851 register defines.
#include "epdPmicDefs.h"

class EpdPmic
{
    public:
        EpdPmic();
        bool begin();
        void setRails(uint8_t _rails);
        void setVCOM(double vcom);
        double getVCOM();
        void setPowerOnSeq(uint8_t _upSeq, uint8_t _upSeqDelay);
        void setPowerOffSeq(uint8_t _dwnSeq, uint8_t _dwnSeqDelay);
        bool programVCOM(double _vcom);
        int getTemperature();
        uint8_t getPwrgoodFlag();
        void enableInterrupts(uint16_t _intMask);
        void disableInterrupts(uint16_t _intMask);
        uint16_t getIntStatus();
        void voltageAdjust(uint8_t _vAdj);

    private:
        void readRegister(uint8_t _reg, uint8_t *_data, uint8_t _n);
        void writeRegister(uint8_t _reg, uint8_t *_data, uint8_t _n);
};

#endif