// Inclucde header the file.
#include "helpers.h"

void Helpers::copySDRAMBuffers(MDMA_HandleTypeDef *hmdma, uint8_t *_internalBuffer, uint32_t _internalBufferSize, volatile uint8_t *_srcBuffer, volatile uint8_t *_destBuffer, uint32_t _size)
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
        while(stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();

        // Increment the pointers.
        _srcBuffer += _internalBufferSize;
        _destBuffer += _internalBufferSize;
    }

    // Copy the remainder of the last chunk.
    if (_lastChinkSize != 0)
    {
        HAL_MDMA_Start_IT(hmdma, (uint32_t)_srcBuffer, (uint32_t)_destBuffer, _lastChinkSize, 1);
        while(stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();
    }
}