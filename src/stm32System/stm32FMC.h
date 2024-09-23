/**
 **************************************************
 *
 * @file        stm32FMC.h
 * @brief       Main header file for the STM32 FMC peripheral
 *              used for SDRAM and ePaper driver. Handles FMC
 *              peripheral initialization/deinitialization, timings,
 *              Master DMA init, Master DMA interrupts etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Add a header guard to the library.
#ifndef __STM32FMC_H__
#define __STM32FMC_H__

// Include main header file for the Arduino.
#include "Arduino.h"

// Include STM32 SRAM HAL functions.
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_sram.h"
#include "stm32h7xx_hal_sdram.h"
#include "stm32h7xx_hal_mdma.h"

// Needed for Debug messages
#include "../system/defines.h"

// STM32 HAL functions for initialization of the FMP Peripheral hardware (GPIO pins, clocks, etc).
// Must be defined as extern for HAL driver to be able to find them.
extern "C" void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram);
extern "C" void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef *hsram);
extern "C" void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram);

void stm32FmcInit(uint32_t _ePaperPeriphAddress);
void stm32FmcDeInit();
void stm32FmcMpuInit(uint32_t _epaperPeriphAddress);
void stm32FmcMdmaInit();
SRAM_HandleTypeDef *stm32FmcGetEpdInstance();
SDRAM_HandleTypeDef *stm32FmcGetSdramInstance();
MDMA_HandleTypeDef *stm32FmcGetEpdMdmaInstance();
MDMA_HandleTypeDef *stm32FmcGetSdramMdmaInstance();
MPU_Region_InitTypeDef *stm32FmcGetMpuInstance();
void stm32FmcSdramTransferCompleteCallback(MDMA_HandleTypeDef *_mdma);
void stm32FmcEpdTransferCompleteCallback(MDMA_HandleTypeDef *_mdma);
void stm32FmcClearEpdCompleteFlag();
void stm32FmcClearSdramCompleteFlag();
uint8_t stm32FmcEpdCompleteFlag();
uint8_t stm32FmcSdramCompleteFlag();
extern "C" void MDMA_IRQHandler();

#endif