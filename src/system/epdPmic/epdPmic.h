/**
 **************************************************
 *
 * @file        epdPmic.h
 * @brief       Header file for the ePaper power managment IC library (TPS658151).
 *              Library includes all methods to communicate wtih the IC including
 *              power up/power down, temperature reading, VCOM set and VCOM programming,
 *              voltage correction, rail enable/disable etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Add header guard for the library.
#ifndef __EPDPMIC_H__
#define __EPDPMIC_H__

// Include Arduino Header file.
#include "Arduino.h"

// Include Arduino I2C library
#include <Wire.h>

// Include all TPS651851 register defines.
#include "epdPmicDefs.h"

// Create the PMIC (TPS658151) class.
class EpdPmic
{
  public:
    // ePaper PMIC constructor.
    EpdPmic();

    // Library initializer.
    bool begin();

    // Enable/disable the powet rails of the TPS651851.
    void setRails(uint8_t _rails);

    // Set VCOM voltage without programming.
    void setVCOM(double vcom);

    // Get VCOM voltage from the TPS651851.
    double getVCOM();

    // Set power on sequence of the power rails of the TPS651851 and it's delays.
    void setPowerOnSeq(uint8_t _upSeq, uint8_t _upSeqDelay);

    // Set power off sequence of the power rails of the TPS651851 and it's delays.
    void setPowerOffSeq(uint8_t _dwnSeq, uint8_t _dwnSeqDelay);

    // Program VCOM (and possibly power on and off sequence, rails?) in the TPS651851 EEPROM memory.
    // NOTE!!! This EEPROM can only be repogrammed 100 times!
    bool programVCOM(double _vcom);

    // Get the temperature from the TPS651851 PMIC temperature sensor.
    int getTemperature();

    // Check the status of the each power rail.
    uint8_t getPwrgoodFlag();

    // Enable selected interrupts.
    void enableInterrupts(uint16_t _intMask);

    // Disable selected interrutps.
    void disableInterrupts(uint16_t _intMask);

    // Check the status of each interrupt.
    uint16_t getIntStatus();

    // Make a foltage adjustment on VPOS & VNEG rails.
    void voltageAdjust(uint8_t _vAdj);

  private:
    // Data access methods for the TPS658151 IC.
    void readRegister(uint8_t _reg, uint8_t *_data, uint8_t _n);
    void writeRegister(uint8_t _reg, uint8_t *_data, uint8_t _n);
};

#endif