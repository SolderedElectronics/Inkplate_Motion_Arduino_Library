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

// Define SRAM start address
#define EXTERNAL_SRAM_ADDR  0x60000000

// Define EPD FMC Peripheral address
#define EPD_FMC_ADDR        0x68000000

// STM32 HAL functions for initialization of the FMP Peripheral hardware (GPIO pins, clocks, etc).
// Must be defined as extern for HAL driver to be able to find them.
extern "C" void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram);
extern "C" void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef *hsram);
extern "C" void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram);

void stm32FmcInit();
void stm32MpuInit();
void stm32MDMAInit();
void stm32FMCSRAMTransferCompleteCallback(MDMA_HandleTypeDef *_mdma);
void stm32FMCEPDTransferCompleteCallback(MDMA_HandleTypeDef *_mdma);
void stm32FMCClearEPDCompleteFlag();
void stm32FMCClearSRAMCompleteFlag();
uint8_t stm32FMCEPDCompleteFlag();
uint8_t stm32FMCSRAMCompleteFlag();
extern "C" void MDMA_IRQHandler();

#endif