/**
 **************************************************
 *
 * @file        helpers.cpp
 * @brief       Source file for the helpers functions.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Inclucde header the file.
#include "helpers.h"

/**
 * @brief   Function copy part of the SDRAM and writes it into dedicated DMA buffers.
 *          Transfer is done using MDMA.
 *
 * @param   MDMA_HandleTypeDef *hmdma
 *          STM32 Master DMA pointer to the instance/typedef.
 * @param   uint8_t *_internalBuffer
 *          Internal buffer used for DMA copying. MUST BE INSIDE DTCMRAM!
 * @param   uint32_t _internalBufferSize
 *          Size of the buffer used for reading chunk-by-chunk in bytes.
 *          Recommended size of the buffer is 8kB (8192 bytes).
 * @param   volatile uint8_t *_srcBuffer
 *          Address of the source buffer (SDRAM).
 * @param   volatile uint8_t *_destBuffer
 *          Pointer to the destiantion buffer.
 * @param   uint32_t _size
 *          Total size of the read (in bytes).
 *
 */
void Helpers::copySDRAMBuffers(MDMA_HandleTypeDef *hmdma, uint8_t *_internalBuffer, uint32_t _internalBufferSize,
                               volatile uint8_t *_srcBuffer, volatile uint8_t *_destBuffer, uint32_t _size)
{
    // Calculate how many memory segments there are.
    uint16_t _chunks = _size / _internalBufferSize;

    // Calculate the reminder (last chunk size).
    uint16_t _lastChinkSize = _size % _internalBufferSize;

    // Align chunks to be in 32 bits size.
    _lastChinkSize = MULTIPLE_OF_4(_lastChinkSize);

    for (uint16_t i = 0; i < _chunks; i++)
    {
        // Start DMA transfer from SDRAM to internal STM32 SRAM.
        HAL_MDMA_Start_IT(hmdma, (uint32_t)_srcBuffer, (uint32_t)_destBuffer, _internalBufferSize, 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();

        // Increment the pointers.
        _srcBuffer += _internalBufferSize;
        _destBuffer += _internalBufferSize;
    }

    // Copy the remainder of the last chunk.
    if (_lastChinkSize != 0)
    {
        HAL_MDMA_Start_IT(hmdma, (uint32_t)_srcBuffer, (uint32_t)_destBuffer, _lastChinkSize, 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();
    }
}