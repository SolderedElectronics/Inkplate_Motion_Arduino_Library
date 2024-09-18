/**
 **************************************************
 * @file        Pcal.cpp
 * @brief       class for controling pcal expander
 *
 *              https://github.com/e-radionicacom/Inkplate-Arduino-library
 *              For support, please reach over forums: forum.e-radionica.com/en
 *              For more info about the product, please check: www.inkplate.io
 *
 *              This code is released under the GNU Lesser General Public
 *License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html Please review the
 *LICENSE file included with this example. If you have any questions about
 *licensing, please contact techsupport@e-radionica.com Distributed as-is; no
 *warranty is given.
 *
 * @authors     Soldered
 ***************************************************/

#include "PCAL_IO.h"

/**
 * @brief       ioBegin function starts pcal expander and sets registers values
 *
 * @param       uint8_t _addr
 *              IO Exapnder I2C address
 *
 * @return      true if successful, false otherwise
 */
bool IOExpander::beginIO(uint8_t _addr)
{
    // Copy the address into local variable.
    _ioExpanderI2CAddress = _addr;

    Wire.beginTransmission(_ioExpanderI2CAddress);
    int error = Wire.endTransmission();
    if (error)
        return false;
    readPCALRegisters();
    return true;
}

/**
 * @brief       readPCALRegisters function uses I2C to read all PCAL I/O expander registers
 *              to avoid read-modify-write.
 *
 */
void IOExpander::readPCALRegisters()
{
    Wire.beginTransmission(_ioExpanderI2CAddress);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(_ioExpanderI2CAddress, (uint8_t)23);
    for (int i = 0; i < 23; i++)
    {
        _ioExpanderRegs[i] = Wire.read();
    }
}

/**
 * @brief       readPCALRegisters function uses I2C to read selected pcal
 * registers
 *
 * @param       uint8_t _regIndex
 *              Start index of the PCAL6416 registers
 * @param       uint8_t _n
 *              number of bites/registers to read
 */
void IOExpander::readPCALRegisters(uint8_t _regIndex, uint8_t _n)
{
    Wire.beginTransmission(_ioExpanderI2CAddress);
    Wire.write(regAddresses[_regIndex]);
    Wire.endTransmission();
    Wire.requestFrom(_ioExpanderI2CAddress, _n);
    for (int i = 0; i < _n; i++)
    {
        _ioExpanderRegs[_regIndex + i] = Wire.read();
    }
}

/**
 * @brief       readPCALRegisters function uses I2C to read one selected pcal
 * register
 *
 * @param       uint8_t _regIndex
 *              Start index of the PCAL6416 registers
 */
void IOExpander::readPCALRegister(uint8_t _regIndex)
{
    Wire.beginTransmission(_ioExpanderI2CAddress);
    Wire.write(regAddresses[_regIndex]);
    Wire.endTransmission();
    Wire.requestFrom(_ioExpanderI2CAddress, (uint8_t)1);
    _ioExpanderRegs[_regIndex] = Wire.read();
}

/**
 * @brief       updatePCALAllRegisters function uses I2C to updates all pcal
 * registers
 *
 */
void IOExpander::updatePCALAllRegisters()
{
    Wire.beginTransmission(_ioExpanderI2CAddress);
    Wire.write(0x00);
    for (int i = 0; i < 23; i++)
    {
        Wire.write(_ioExpanderRegs[i]);
    }
    Wire.endTransmission();
}

/**
 * @brief       updatePCALRegister function uses I2C to update selected pcal register
 *
 * @param       uint8_t _regIndex
 *              Start index of the PCAL6416 registers
 * @param       uint8_t _d
 *              data to be uploaded
 */
void IOExpander::updatePCALRegister(uint8_t _regIndex, uint8_t _d)
{
    Wire.beginTransmission(_ioExpanderI2CAddress);
    Wire.write(regAddresses[_regIndex]);
    Wire.write(_d);
    Wire.endTransmission();
}

/**
 * @brief       pinModeIO function sets internal pin mode
 *
 * @param       uint8_t _pin
 *              pin to set mode
 * @param       uint8_t _mode
 *              mode for pin to be set (INPUT=0x01, OUTPUT=0x02,
 * INPUT_PULLUP=0x05)
 * @param       bool _bypassCheck
 *              Setting this to true will bypass user block on this GPIO pin.
 */
void IOExpander::pinModeIO(uint8_t _pin, uint8_t _mode, bool _bypassCheck)
{
    // If the usage of the pin is blocked, return without register modify.
    if (checkForBlockedPins(_pin) && !_bypassCheck) return;

    // Pin not blocked? Set new pin mode.
    pinModeInternal(_pin, _mode);
}

/**
 * @brief       digitalWriteIO sets internal output pin state (1 or 0)
 *
 * @param       uint8_t _pin
 *              pin to set output (DO NOT USE GPA0-GPA7 and GPB0. In code those
 * are pins from 0-8) only use 9-15
 * @param       uint8_t _state
 *              output pin state (0 or 1)
 * @param       bool _bypassCheck
 *              Setting this to true will bypass user block on this GPIO pin.
 *
 */
void IOExpander::digitalWriteIO(uint8_t _pin, uint8_t _state, bool _bypassCheck)
{
    // If the usage of the pin is blocked, return without register modify.
    if (checkForBlockedPins(_pin) && !_bypassCheck) return;

    // Pin not blocked? Set new state of the pin.
    digitalWriteInternal(_pin, _state);
}

/**
 * @brief       digitalReadPCAL reads pcal internal pin state
 *
 * @param       uint8_t _pin
 *              pin to set mode
 * @param       bool _bypassCheck
 *              Setting this to true will bypass user block on this GPIO pin.
 * 
 * @return      HIGH or LOW (1 or 0) value
 */
uint8_t IOExpander::digitalReadIO(uint8_t _pin, bool _bypassCheck)
{
    // If the usage of the pin is blocked, return without register modify.
    if (checkForBlockedPins(_pin) && !_bypassCheck) return 0;

    // Pin not blocked? Read the sate of the pin.
    return digitalReadInternal(_pin);
}

/**
 * @brief       setIntPin function enables interrupt on change on IO Expander pin.
 *
 * @param       uint8_t _pin
 *              pin to set interrupt mode to
 */
void IOExpander::setIntPinIO(uint8_t _pin)
{
    setIntPinInternal(_pin);
}

/**
 * @brief       getINTInternal function reads did interrupt on change event has occur.
 *
 * @return      returns interupt registers state
 *
 * @note        Every bit represents interrupt pin, MSB is  PORTB PIN7, LSB is
 * PORTA PIN1
 */
uint16_t IOExpander::getIntIO()
{
    return getINTInternal();
}

/**
 * @brief       removeIntPin function removes Interrupt from pin
 *
 * @param       uint8_t _pin
 *              pin to remove interrupt from
 */
void IOExpander::removeIntPinIO(uint8_t _pin)
{
    removeIntPinInternal(_pin);
}

/**
 * @brief       setPorts sets states on every IO Expander pin at once.
 *
 * @param       uint16_t _d
 *              GPIO pin state of all IO Expander pins.
 *
 */
void IOExpander::setPortsIO(uint16_t _d)
{
    setPortsInternal(_d);
}

/**
 * @brief       getPorts reads GPIO pin state on every IO Expander pin at once.
 *
 */
uint16_t IOExpander::getPortsIO()
{
    return getPortsInternal();
}

/**
 * @brief   Sets block on specific pin so user could not use it. This is to prevent damage to the
 *          board since some of the GPIO pins from the I/O expander are connected to something on the board
 *          (for example TPS).
 * 
 * @param   uint8_t _pin
 *          I/O exapnder GPIO pin (IO_PIN_A0 - IO_PIN_A7, IO_PIN_B0 - 
 *          IO_PIN_B7).
 */
void IOExpander::blockPinUsage(uint8_t _pin)
{
    // Only pins from 0 to 15 are allowed.
    _pin &= 15;

    // Update the table.
    _blockedPinsForUser |= 1ULL << _pin;
}

/**
 * @brief   Remove block on specific pin so user could use it.
 * 
 * @param   uint8_t _pin
 *          I/O exapnder GPIO pin (IO_PIN_A0 - IO_PIN_A7, IO_PIN_B0 - 
 *          IO_PIN_B7).
 */
void IOExpander::unblockPinUsage(uint8_t _pin)
{
    // Only pins from 0 to 15 are allowed.
    _pin &= 15;

    // Update the table.
    _blockedPinsForUser &= ~(1ULL << _pin);
}

/**
 * @brief       pinModeInternal sets IO Exapnder internal pin mode
 *
 * @param       uint8_t _pin
 *              pin to set mode
 * @param       uint8_t _mode
 *              mode for pi to be set (INPUT=0x01, OUTPUT=0x02,
 * INPUT_PULLUP=0x05)
 *
 * @note        modes are defined in @esp32-hal-gpio.h
 */
void IOExpander::pinModeInternal(uint8_t _pin, uint8_t _mode)
{
    if (_pin > 15)
        return;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    switch (_mode)
    {
    case INPUT:
        _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        updatePCALRegister(PCAL6416A_CFGPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port]);
        break;
    case OUTPUT:
        // There is a one cacth! Pins are by default (POR) set as HIGH. So first change it to LOW and then set is as
        // output).
        _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port] &= ~(1 << _pin);
        _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY + _port] &= ~(1 << _pin);
        updatePCALRegister(PCAL6416A_OUTPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY + _port]);
        updatePCALRegister(PCAL6416A_CFGPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port]);
        break;
    case INPUT_PULLUP:
        _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        _ioExpanderRegs[PCAL6416A_PUPDEN_REG0_ARRAY + _port] |= (1 << _pin);
        _ioExpanderRegs[PCAL6416A_PUPDSEL_REG0_ARRAY + _port] |= (1 << _pin);
        updatePCALRegister(PCAL6416A_CFGPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port]);
        updatePCALRegister(PCAL6416A_PUPDEN_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_PUPDEN_REG0_ARRAY + _port]);
        updatePCALRegister(PCAL6416A_PUPDSEL_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_PUPDSEL_REG0_ARRAY + _port]);
        break;
    case INPUT_PULLDOWN:
        _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port] |= (1 << _pin);
        _ioExpanderRegs[PCAL6416A_PUPDEN_REG0_ARRAY + _port] |= (1 << _pin);
        _ioExpanderRegs[PCAL6416A_PUPDSEL_REG0_ARRAY + _port] &= ~(1 << _pin);
        updatePCALRegister(PCAL6416A_CFGPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_CFGPORT0_ARRAY + _port]);
        updatePCALRegister(PCAL6416A_PUPDEN_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_PUPDEN_REG0_ARRAY + _port]);
        updatePCALRegister(PCAL6416A_PUPDSEL_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_PUPDSEL_REG0_ARRAY + _port]);
        break;
    }
}

/**
 * @brief       digitalWriteInternal sets internal output pin state (1 or 0)

 * @param       uint8_t _pin
 *              pin to set output (DO NOT USE GPA0-GPA7 and GPB0. In code those
 * are pins from 0-8) only use 9-15
 * @param       uint8_t _state
 *              output pin state (0 or 1)
 *
 * @note        DO NOT USE GPA0-GPA7 and GPB0. In code those are pins from
 * 0-8!!! Using those, you might permanently damage the screen. You should only
 * use pins from 9-15. Function will exit if pin mode isnt OUTPUT.
 */
void IOExpander::digitalWriteInternal(uint8_t _pin, uint8_t _state)
{
    if (_pin > 15)
        return;
    _state &= 1;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    _state ? _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY + _port] |= (1 << _pin) : _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY + _port] &= ~(1 << _pin);
    updatePCALRegister(PCAL6416A_OUTPORT0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY + _port]);
}

/**
 * @brief       digitalReadInternal reads io exapnder internal pin state
 * 
 *              pointer to array that holds io exapnder registers
 * @param       uint8_t _pin
 *              pin to set mode
 *
 * @return      HIGH or LOW (1 or 0) value
 */
uint8_t IOExpander::digitalReadInternal(uint8_t _pin)
{
    if (_pin > 15)
        return -1;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    readPCALRegister(PCAL6416A_INPORT0_ARRAY + _port);
    return ((_ioExpanderRegs[PCAL6416A_INPORT0_ARRAY + _port] >> _pin) & 1);
}

/**
 * @brief       setIntPinInternal function sets Interrupt on selected pin
 *
 *              pointer to array that holds IO Exapnder registers
 * @param       uint8_t *_pin
 *              selected pin
 */
void IOExpander::setIntPinInternal(uint8_t _pin)
{
    if (_pin > 15)
        return;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    _ioExpanderRegs[PCAL6416A_INTMSK_REG0_ARRAY + _port] &= ~(1 << _pin);

    updatePCALRegister(PCAL6416A_INTMSK_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_INTMSK_REG0_ARRAY + _port]);
}

/**
 * @brief       removeIntPinInternal function removes Interrupt on selected pin
 *
 *              pointer to array that holds io exapnder registers
 * @param       uint8_t *_pin
 *              selected pin
 */
void IOExpander::removeIntPinInternal(uint8_t _pin)
{
    if (_pin > 15)
        return;

    uint8_t _port = _pin / 8;
    _pin %= 8;

    _ioExpanderRegs[PCAL6416A_INTMSK_REG0_ARRAY + _port] |= (1 << _pin);

    updatePCALRegister(PCAL6416A_INTMSK_REG0_ARRAY + _port, _ioExpanderRegs[PCAL6416A_INTMSK_REG0_ARRAY + _port]);
}

/**
 * @brief       getINTInternal function reads Interrupt pin state for all pins
 *
 *              pointer to array that holds io exapnder registers
 * @return      returns interrupt state of both ports (INTF)
 *
 * @note        Every bit represents interrupt pin, MSB is  PORTB PIN7, LSB is
 * PORTA PIN1, bit can be set only if interrupt is enabled
 */
uint16_t IOExpander::getINTInternal()
{
    readPCALRegister(PCAL6416A_INTSTAT_REG0_ARRAY);
    readPCALRegister(PCAL6416A_INTSTAT_REG1_ARRAY);

    return ((_ioExpanderRegs[PCAL6416A_INTSTAT_REG1_ARRAY] << 8) | (_ioExpanderRegs[PCAL6416A_INTSTAT_REG0_ARRAY]));
}

/**
 * @brief       setPortsInternal sets all pins at once.
 *
 *              pointer to array that holds io exapnder registers
 * @param       uint6_t _d
 *              GPIO data. Every bit represents one GPIO pin. Writing one to one bit sets pin on high logic level (is
 * selected pin is set as GPIO).
 */
void IOExpander::setPortsInternal(uint16_t _d)
{
    _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY] = (_d & 0xff);
    _ioExpanderRegs[PCAL6416A_OUTPORT1_ARRAY] = (_d >> 8) & 0xff;
    updatePCALRegister(PCAL6416A_OUTPORT0_ARRAY, _ioExpanderRegs[PCAL6416A_OUTPORT0_ARRAY]);
    updatePCALRegister(PCAL6416A_OUTPORT1_ARRAY, _ioExpanderRegs[PCAL6416A_OUTPORT1_ARRAY]);
}

/**
 * @brief       getPortsInternal reads all pins at once.
 *
 *
 * @return      GPIO data. Every bit represents one GPIO pin. Reading one to one bit means selected GPIO has high logic
 * state.
 */
uint16_t IOExpander::getPortsInternal()
{
    readPCALRegister(PCAL6416A_INPORT0_ARRAY);
    readPCALRegister(PCAL6416A_INPORT1_ARRAY);

    return (_ioExpanderRegs[PCAL6416A_INPORT0_ARRAY] | (_ioExpanderRegs[PCAL6416A_INPORT1_ARRAY]) << 8);
}

/**
 * @brief   Checks for the pins taht users are not allowed to use.
 * 
 * @param   uint8_t _pin
 *          GPIO pin on the I/O expander
 * @return  bool    
 *          true - Pin in blocked for the user - user must not use this pin (it's internally
 *          connected to something on the board, check the schematic!
 * @return  false - User can use this pin.
 */
bool IOExpander::checkForBlockedPins(uint8_t _pin)
{
    // Check if the pin usage for this specific pin is blocked for user comparing it with the internal table.
    if (_blockedPinsForUser & (1ULL << _pin)) return true;

    // Pin is not blocked, user can use it.
    return false;
}