#include "epdPmic.h"

EpdPmic::EpdPmic()
{
    // Empty constructor.
}

bool EpdPmic::begin()
{
    // Try to ping PMIC. Return false if failed.
    Wire.beginTransmission(TPS_PMIC_ADDR);
    int _ret = Wire.endTransmission();

    // If Wire.endTransmission returns anything else than 0 - Success, return false.
    return (_ret != 0?false:true);
}

void EpdPmic::setRails(uint8_t _rails)
{
    // Disable or enable rails on the PMIC.
    
    // Remove upper two bits.
    _rails &= 0b00111111;

    // Send the data to the PMIC.
    writeRegister(TPS651851_ENABLE, &_rails, 1);
}

void EpdPmic::setVCOM(double _vcom)
{
    // Array for VCOM registers.
    uint8_t _vcomRegs[2] = {0, 0};

    // First divide VCOM voltage by 100 and remove the "-" sign.
    _vcom = abs(_vcom) * 100;

    // Get the VCOM2 register
    readRegister(TPS651851_VCOM2, &_vcomRegs[1], 1);

    // Save lower 8 bits into VCOM1
    _vcomRegs[0] = (int)(_vcom);

    // Save upper 9th bit of the VCOM into bitst bit of the VCOM2 register.
    _vcomRegs[1] &= 0b11111110;
    _vcomRegs[1] |= ((int)_vcom >> 8) & 1;

    // Write data to the PMIC.
    writeRegister(TPS651851_VCOM1, _vcomRegs, 2);
}

double EpdPmic::getVCOM()
{
    // Array to store the content of the VCOM registers.
    uint8_t _vcomRegs[2] = {0, 0};

    // Variable to store calculated VCOM voltage in volts.
    double _vcomVolts = 0;

    // Get the register values.
    readRegister(TPS651851_VCOM1, _vcomRegs, 2);

    // Convert integer value into volts.
    _vcomVolts = (_vcomRegs[0] | ((_vcomRegs[1] & 1) << 8)) / 100.0;
    
    // Return the value and add "-" sign.
    return (_vcomVolts * (-1));
}

void EpdPmic::setPowerOnSeq(uint8_t _upSeq, uint8_t _upSeqDelay)
{
    // Array to store register values.
    uint8_t _upSeqRegs[2];

    // Store power up sequence into first byte.
    _upSeqRegs[0] = _upSeq;

    // Store power up delays into second register.
    _upSeqRegs[1] = _upSeqDelay;

    // Send the regs to the TPS PMIC.
    writeRegister(TPS651851_UPSEQ0, _upSeqRegs, 2);
}

void EpdPmic::setPowerOffSeq(uint8_t _dwnSeq, uint8_t _dwnSeqDelay)
{
    // Array to store register values.
    uint8_t _dwnSeqRegs[2];

    // Store power down sequence into first byte.
    _dwnSeqRegs[0] = _dwnSeq;

    // Store power down delays into second register.
    _dwnSeqRegs[1] = _dwnSeqDelay;

    // Send the regs to the TPS PMIC.
    writeRegister(TPS651851_DWNSEQ0, _dwnSeqRegs, 2);
}

bool EpdPmic::programVCOM(double _vcom)
{
    // Variable for VCOM programming flag. Set the default value to 0x04
    // (TPS651851 reset value).
    uint8_t _vcom2Reg = 0x04;

    // Variable holds INT status (needed for EEPROM programming complete).
    uint16_t _intStatusFlags;

    // EEPROM write timeout variable value.
    unsigned long _eepromWriteTimeout;

    // First set the VCOM value.
    setVCOM(_vcom);

    // Set VCOM programmming flag to active!
    // Read the VCOM2 register.
    readRegister(TPS651851_VCOM2, &_vcom2Reg, 1);

    // Modify the value (set PROG bit to 1).
    _vcom2Reg |= (1 << 6);

    // Send this new VCOM2 register value to the TPS.
    writeRegister(TPS651851_VCOM2, &_vcom2Reg, 1);

    // Wait EEPROM Burn to finish.
    // Capture the timestamp!
    _eepromWriteTimeout = millis();
    do
    {
        // Read the new values of the Interrupt status flags.
        _intStatusFlags = getIntStatus();

        // Wait a little bit before new status reading.
        delay(5);
    } while (((unsigned long)(millis() - _eepromWriteTimeout) <= 1000ULL) & !(_intStatusFlags & 1));

    // Check for EREPROM programming success (timeout did not occured).
    if (_intStatusFlags & 1)
    {
        return true;
    }
    
    // If EEPROM programming failed, return false.
    return false;
}

int EpdPmic::getTemperature()
{
    // Temp. variable for storing temperature data.
    uint8_t _temp = 0;

    // Start temperature measurement. Get TMST1 register value.
    // Set default value, just in case.
    uint8_t _tempConfReg = 0x20;
    
    // Read the value of the register.
    readRegister(TPS651851_TMST1, &_tempConfReg, 1);

    // Modify it to start measurement.
    _tempConfReg |= (1 << 7);
    writeRegister(TPS651851_TMST1, &_tempConfReg, 1);

    // Wait until conversion is complete. Set the timeout value of 100ms.
    unsigned long _thermistorTimeout = millis();
    do
    {
        // Read status flag.
        readRegister(TPS651851_TMST1, &_tempConfReg, 1);

        // Wait a little bit.
        delay(5);
    } while (((unsigned long)(millis() - _thermistorTimeout) < 100ULL) && !(_tempConfReg & (1 << 5)));

    // Check for the conversion end event.
    if (!_tempConfReg & (1 << 5)) return 0;

    // Get the register value from the TPS PMIC.
    readRegister(TPS651851_TMST_VALUE, &_temp, 1);

    // Return the value.
    return (int8_t)(_temp);
}

uint8_t EpdPmic::getPwrgoodFlag()
{
    // Temp. variable for storing power good flag.
    uint8_t _pwrGood = 0;

    // Get the register value from TPS PMIC.
    readRegister(TPS651851_PG, &_pwrGood, 1);

    // Return the value.
    return _pwrGood;
}

void EpdPmic::enableInterrupts(uint16_t _intMask)
{
    // Get the INT register value from the TPS.
    uint16_t _intEnableRegs = 0;
    readRegister(TPS651851_INT_EN1, (uint8_t*)(&_intEnableRegs), 2);

    // Modify the value of the registers.
    _intEnableRegs |= _intMask;

    // Send back modified register.
    writeRegister(TPS651851_INT_EN1, (uint8_t*)(&_intEnableRegs), 2);
}

void EpdPmic::disableInterrupts(uint16_t _intMask)
{
    // Get the INT register value from the TPS.
    uint16_t _intEnableRegs = 0;
    readRegister(TPS651851_INT_EN1, (uint8_t*)(&_intEnableRegs), 2);

    // Modify the value of the registers.
    _intEnableRegs &= ~(_intMask);

    // Send back modified register.
    writeRegister(TPS651851_INT_EN1, (uint8_t*)(&_intEnableRegs), 2);
}

void EpdPmic::voltageAdjust(uint8_t _vAdj)
{
    // Mask the data.
    _vAdj &= 0b00000111;

    // Check for the unvalid data.
    if ((_vAdj < 3) && (_vAdj > 6)) return;

    // Read the register from the TPS651851.
    uint8_t _vAdjReg = 0x23;
    readRegister(TPS651851_VADJ, &_vAdjReg, 1);

    // Modify the register.
    _vAdjReg &= ~0b00000111;
    _vAdjReg |= _vAdj;

    // Send it back to the TPS651851.
    writeRegister(TPS651851_VADJ, &_vAdjReg, 1);
}

uint16_t EpdPmic::getIntStatus()
{
    // Read the current INT status values from the TPS.
    uint16_t _intStatusRegs = 0;
    readRegister(TPS651851_INT1, (uint8_t*)(&_intStatusRegs), 2);

    // Return the values.
    return _intStatusRegs;
}

void EpdPmic::readRegister(uint8_t _reg, uint8_t *_data, uint8_t _n)
{
    // Set the register address.
    Wire.beginTransmission(TPS_PMIC_ADDR);
    Wire.write(_reg);
    Wire.endTransmission();

    // Read the register content.
    Wire.requestFrom(TPS_PMIC_ADDR, _n);
    while (_n--)
    {
        _data[_n] = Wire.read();
    }
}

void EpdPmic::writeRegister(uint8_t _reg, uint8_t *_data, uint8_t _n)
{
    // Set the register address.
    Wire.beginTransmission(TPS_PMIC_ADDR);
    Wire.write(_reg);

    // Write the data to the register.
    Wire.write(_data, _n);

    // Finish I2C transmission.
    Wire.endTransmission();
}