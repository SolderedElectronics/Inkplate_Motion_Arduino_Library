/**
 **************************************************
 *
 * @file        helpers.h
 * @brief       Header file for the helpers functions.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#ifndef _INKPLATE_HELPERS_H__
#define _INKPLATE_HELPERS_H__

// Include main Arduino Header file.
#include "Arduino.h"

// Include custom library for the STM32 FMC.
#include "../stm32System/stm32FMC.h"

// Macro function to round array size to be multiple of 4 (needed for DMA controller, since it works with 32 bit data
// values).
#define MULTIPLE_OF_4(x) (((x - 1) | 3) + 1)

// Fast conversion from Wavefrom to EPD data.
/**
 * @brief   Fast conversion from Wavefrom to EPD data.
 *
 * @param   uint8_t _epdByte
 *          Used for epaper clean function. Allowed values (0, 1, 2, 3).
 *
 */
__attribute__((always_inline)) static inline uint8_t wavefromElementToEpdData(uint8_t _epdByte)
{
    // Limit byte from 0 to 3.
    _epdByte &= 0x03;

    // Variable to store the EPD data.
    uint8_t _epdData = 0;

    // Set output data.
    switch (_epdByte)
    {
    case 0:
        _epdData = 0b00000000; // Discharge
        break;
    case 1:
        _epdData = 0b01010101; // Black
        break;
    case 2:
        _epdData = 0b10101010; // White
        break;
    case 3:
        _epdData = 0b11111111; // Skip
        break;
    }

    // Return the EPD data.
    return _epdData;
}


class Helpers
{
  public:
    Helpers();
    // Function is used to copy
    static void copySDRAMBuffers(MDMA_HandleTypeDef *hmdma, uint8_t *_internalBuffer, uint32_t _internalBufferSize,
                                 volatile uint8_t *_srcBuffer, volatile uint8_t *_destBuffer, uint32_t _size);

  private:
};

#endif