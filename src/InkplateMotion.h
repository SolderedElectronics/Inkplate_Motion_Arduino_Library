/**
 **************************************************
 *
 * @file        InkplateMotion.cpp
 * @brief       Main source file of the Inkplate library.
 *              Contains main function and methods linked to
 *              the driver code.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Add header guard.
#ifndef __INKPLATEMOTION_H__
#define __INKPLATEMOTION_H__

// Include main Arduino Header file.
#include "Arduino.h"

// Include files for Inkplate driver
#include "system/InkplateBoards.h"

// Include Adafruit GFX, I2C and SPI libary
#include "SPI.h"
#include "Wire.h"
#include "libs/Adafruit-GFX-Library/Adafruit_GFX.h"

// Include library defines
#include "system/defines.h"

// Include custom RTC library for STM32.
#include "stm32System/STM32H7RTC.h"

// Include WiFi Library for the ESP32 (using AT commands over SPI).
#include "system/wifi/esp32SpiAt.h"

// Include header file for board select.
#include "boardSelect.h"

// Include file for the each board feature selection.
#include "features/featureSelect.h"

class Inkplate : public Adafruit_GFX, public InkplateBoardClass
{
  public:
    Inkplate();
    void begin(uint8_t _mode = INKPLATE_1BW);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void setRotation(uint8_t);
    void drawBitmap4Bit(int16_t _x, int16_t _y, const unsigned char *_p, int16_t _w, int16_t _h);

  protected:
  private:
    uint8_t _rotation = 0;
    uint8_t _beginDone = 0;
};

#endif
