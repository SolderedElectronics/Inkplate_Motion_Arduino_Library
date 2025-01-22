/**
 **************************************************
 *
 * @file        pcalExpander.h
 * @brief       Header file for the PCAL6416 GPIO Expander control.
 *              Enables expanding number of GPIOs with I2C commuinication.
 *              Mostly used for Inkplate peripherals.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#ifndef __PCAL_EXPANDER_H__
#define __PCAL_EXPANDER_H__

#include "Arduino.h"
#include "Wire.h"

// PCAL6416 Register Adresses
#define PCAL6416A_INPORT0        0x00
#define PCAL6416A_INPORT1        0x01
#define PCAL6416A_OUTPORT0       0x02
#define PCAL6416A_OUTPORT1       0x03
#define PCAL6416A_POLINVPORT0    0x04
#define PCAL6416A_POLINVPORT1    0x05
#define PCAL6416A_CFGPORT0       0x06
#define PCAL6416A_CFGPORT1       0x07
#define PCAL6416A_OUTDRVST_REG00 0x40
#define PCAL6416A_OUTDRVST_REG01 0x41
#define PCAL6416A_OUTDRVST_REG10 0x42
#define PCAL6416A_OUTDRVST_REG11 0x43
#define PCAL6416A_INLAT_REG0     0x44
#define PCAL6416A_INLAT_REG1     0x45
#define PCAL6416A_PUPDEN_REG0    0x46
#define PCAL6416A_PUPDEN_REG1    0x47
#define PCAL6416A_PUPDSEL_REG0   0x48
#define PCAL6416A_PUPDSEL_REG1   0x49
#define PCAL6416A_INTMSK_REG0    0x4A
#define PCAL6416A_INTMSK_REG1    0x4B
#define PCAL6416A_INTSTAT_REG0   0x4C
#define PCAL6416A_INTSTAT_REG1   0x4D
#define PCAL6416A_OUTPORT_CONF   0x4F

// PCAL6416 register index array
#define PCAL6416A_INPORT0_ARRAY        0
#define PCAL6416A_INPORT1_ARRAY        1
#define PCAL6416A_OUTPORT0_ARRAY       2
#define PCAL6416A_OUTPORT1_ARRAY       3
#define PCAL6416A_POLINVPORT0_ARRAY    4
#define PCAL6416A_POLINVPORT1_ARRAY    5
#define PCAL6416A_CFGPORT0_ARRAY       6
#define PCAL6416A_CFGPORT1_ARRAY       7
#define PCAL6416A_OUTDRVST_REG00_ARRAY 8
#define PCAL6416A_OUTDRVST_REG01_ARRAY 9
#define PCAL6416A_OUTDRVST_REG10_ARRAY 10
#define PCAL6416A_OUTDRVST_REG11_ARRAY 11
#define PCAL6416A_INLAT_REG0_ARRAY     12
#define PCAL6416A_INLAT_REG1_ARRAY     13
#define PCAL6416A_PUPDEN_REG0_ARRAY    14
#define PCAL6416A_PUPDEN_REG1_ARRAY    15
#define PCAL6416A_PUPDSEL_REG0_ARRAY   16
#define PCAL6416A_PUPDSEL_REG1_ARRAY   17
#define PCAL6416A_INTMSK_REG0_ARRAY    18
#define PCAL6416A_INTMSK_REG1_ARRAY    19
#define PCAL6416A_INTSTAT_REG0_ARRAY   20
#define PCAL6416A_INTSTAT_REG1_ARRAY   21
#define PCAL6416A_OUTPORT_CONF_ARRAY   22

// User pins on IO Expander for Inkplate 6COLOR
#define IO_PIN_A0 0
#define IO_PIN_A1 1
#define IO_PIN_A2 2
#define IO_PIN_A3 3
#define IO_PIN_A4 4
#define IO_PIN_A5 5
#define IO_PIN_A6 6
#define IO_PIN_A7 7
#define IO_PIN_B0 8
#define IO_PIN_B1 9
#define IO_PIN_B2 10
#define IO_PIN_B3 11
#define IO_PIN_B4 12
#define IO_PIN_B5 13
#define IO_PIN_B6 14
#define IO_PIN_B7 15

/**
 * @brief       PCAL class to be used for work with PCAL6416 expander
 */
class IOExpander
{
  public:
    bool beginIO(uint8_t _addr);
    void pinModeIO(uint8_t _pin, uint8_t _mode, bool _bypassCheck = false);
    void digitalWriteIO(uint8_t _pin, uint8_t _state, bool _bypassCheck = false);
    uint8_t digitalReadIO(uint8_t _pin, bool _bypassCheck = false);
    void setIntPinIO(uint8_t _pin);
    void setPortsIO(uint16_t _d);
    void removeIntPinIO(uint8_t _pin);
    uint16_t getIntIO();
    uint16_t getPortsIO();
    void blockPinUsage(uint8_t _pin);
    void unblockPinUsage(uint8_t _pin);

  private:
    // I/O expander interal register copy to avoid read-modify-write.
    uint8_t _ioExpanderRegs[23];

    // I/O expander registered 7-bit I2C address.
    // Use default as fallback.
    uint8_t _ioExpanderI2CAddress = 0x20;

    // IO expander pins that are blocked for user usage )for safety resons.
    // User can override this by setting _bypassCheck to true AT IT'S OWN RISK!
    uint16_t _blockedPinsForUser = 0;

    const uint8_t regAddresses[23] = {
        PCAL6416A_INPORT0,        PCAL6416A_INPORT1,        PCAL6416A_OUTPORT0,       PCAL6416A_OUTPORT1,
        PCAL6416A_POLINVPORT0,    PCAL6416A_POLINVPORT1,    PCAL6416A_CFGPORT0,       PCAL6416A_CFGPORT1,
        PCAL6416A_OUTDRVST_REG00, PCAL6416A_OUTDRVST_REG01, PCAL6416A_OUTDRVST_REG10, PCAL6416A_OUTDRVST_REG11,
        PCAL6416A_INLAT_REG0,     PCAL6416A_INLAT_REG1,     PCAL6416A_PUPDEN_REG0,    PCAL6416A_PUPDEN_REG1,
        PCAL6416A_PUPDSEL_REG0,   PCAL6416A_PUPDSEL_REG1,   PCAL6416A_INTMSK_REG0,    PCAL6416A_INTMSK_REG1,
        PCAL6416A_INTSTAT_REG0,   PCAL6416A_INTSTAT_REG1,   PCAL6416A_OUTPORT_CONF};

    void pinModeInternal(uint8_t _pin, uint8_t _mode);
    void digitalWriteInternal(uint8_t _pin, uint8_t _state);
    uint8_t digitalReadInternal(uint8_t _pin);
    void setIntPinInternal(uint8_t _pin);
    void removeIntPinInternal(uint8_t _pin);
    uint16_t getINTInternal();
    void setPortsInternal(uint16_t _d);
    uint16_t getPortsInternal();

    void readPCALRegisters();
    void readPCALRegisters(uint8_t _regIndex, uint8_t _n);
    void readPCALRegister(uint8_t _regIndex);
    void updatePCALAllRegisters();
    void updatePCALRegister(uint8_t _regIndex, uint8_t _d);

    bool checkForBlockedPins(uint8_t _pin);
};

#endif