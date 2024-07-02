#ifndef __STM32_MDMA_H__
#define __STM32_MDMA_H__

// Library used for faster DMA transfers (to reduce HAL library overhead).

// Include Arduino Header.
#include "Arduino.h"

// Include HAL library for Master DMA controller.
#include "stm32h7xx_hal_mdma.h"

#include "stm32h7xx_hal_sram.h"

void stm32MDMAConfigure(MDMA_HandleTypeDef *_mdma, uint32_t _sourceAddress, uint32_t _destinationAddress, uint32_t _blockSize);
void stm32MDMAStart(MDMA_HandleTypeDef *_mdma);
void stm32MDMAModifyAddress(MDMA_HandleTypeDef *_mdma, uint32_t _source, uint32_t _dest);
bool stm32MDMATransferComplete(MDMA_HandleTypeDef *_mdma);
bool stm32MDMAClearFlag(MDMA_HandleTypeDef *_mdma);

#endif