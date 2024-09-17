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
void stm32FmcDeInit();
void stm32FmcMpuInit();
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