/**
 **************************************************
 *
 * @file        stm32FMC.cpp
 * @brief       Main source file for the STM32 FMC peripheral
 *              used for SDRAM and ePaper driver. Handles FMC
 *              peripheral initialization/deinitialization, timings,
 *              Master DMA init, Master DMA interrupts etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include header file of this .cpp file
#include "stm32FMC.h"

// FMC HAL Typedefs and init status variables.
// Handle for the FMC LCD interface (for EPD).
SRAM_HandleTypeDef _hsram1;
// Handle for the SDRAM FMC interface.
SDRAM_HandleTypeDef _hsdram1;
// Handle for Master DMA for SDRAM.
MDMA_HandleTypeDef _hmdmaMdmaChannel40Sw0;
// Handle for Master DMA for FMC LCD (EPD).
MDMA_HandleTypeDef _hmdmaMdmaChannel41Sw0;
// Handle memory protection unit.
MPU_Region_InitTypeDef _mpuInitStructEpd;
static uint32_t _stm32FmcInitialized = 0;
static uint32_t _stm32FmcDeInitialized = 0;

// Interrupt flags for MDMA transfer scomplete status.
volatile uint8_t _stm32MdmaEpdCompleteFlag = 0;
volatile uint8_t _stm32MdmaSdramCompleteFlag = 0;

// Really low level STM32 related stuff. Do not change anything unless you really know what you are doing!

/**
 * @brief   FMC initialization function for the STM32 FMC peripheral. This peripheral is used to communicate with the
 *          SDRAM and ePaper parallel bus.
 * 
 */
static void MX_FMC_Init(void)
{
    FMC_NORSRAM_TimingTypeDef _timing = {0};
    FMC_SDRAM_TimingTypeDef _sdramTiming = {0};

    /** Perform the SRAM1 memory initialization sequence
     */
    _hsram1.Instance = FMC_NORSRAM_DEVICE;
    _hsram1.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram1.Init */
    _hsram1.Init.NSBank = FMC_NORSRAM_BANK3;
    _hsram1.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
    _hsram1.Init.MemoryType = FMC_MEMORY_TYPE_SRAM;
    _hsram1.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_8;
    _hsram1.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_DISABLE;
    _hsram1.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    _hsram1.Init.WaitSignalActive = FMC_WAIT_TIMING_BEFORE_WS;
    _hsram1.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
    _hsram1.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
    _hsram1.Init.ExtendedMode = FMC_EXTENDED_MODE_DISABLE;
    _hsram1.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    _hsram1.Init.WriteBurst = FMC_WRITE_BURST_DISABLE;
    _hsram1.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ONLY;
    _hsram1.Init.WriteFifo = FMC_WRITE_FIFO_DISABLE;
    _hsram1.Init.PageSize = FMC_PAGE_SIZE_NONE;
    /* Timing */
    _timing.AddressSetupTime = 0;
    _timing.AddressHoldTime = 0;
    _timing.DataSetupTime = 2;
    _timing.BusTurnAroundDuration = 0;
    _timing.CLKDivision = 1;
    _timing.DataLatency = 1;
    _timing.AccessMode = FMC_ACCESS_MODE_A;
    /* ExtTiming */

    if (HAL_SRAM_Init(&_hsram1, &_timing, NULL) != HAL_OK)
    {
        Error_Handler();
    }

    /** Perform the SDRAM1 memory initialization sequence
     */
    _hsdram1.Instance = FMC_SDRAM_DEVICE;
    /* hsdram1.Init */
    _hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
    _hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
    _hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
    _hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    _hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    _hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
    _hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    _hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    _hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    _hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
    /* SdramTiming */
    _sdramTiming.LoadToActiveDelay = 2;
    _sdramTiming.ExitSelfRefreshDelay = 10;
    _sdramTiming.SelfRefreshTime = 2;
    _sdramTiming.RowCycleDelay = 8;
    _sdramTiming.WriteRecoveryTime = 4;
    _sdramTiming.RPDelay = 2;
    _sdramTiming.RCDDelay = 2;

    if (HAL_SDRAM_Init(&_hsdram1, &_sdramTiming) != HAL_OK)
    {
        Error_Handler( );
    }

    __IO uint32_t _modeRegister         = 0;
    FMC_SDRAM_CommandTypeDef _command  = {0};

    _command.CommandMode               = FMC_SDRAM_CMD_CLK_ENABLE; /* Set MODE bits to "001" */
    _command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2; /* configure the Target Bank bits */
    _command.AutoRefreshNumber         = 1;
    _command.ModeRegisterDefinition    = 0;
    if (HAL_SDRAM_SendCommand(&_hsdram1, &_command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_Delay(1); /* Step 4: Insert 100 us minimum delay - Min HAL Delay is 1ms */

    /* Step 5: Configure a PALL (precharge all) command */
    _command.CommandMode               = FMC_SDRAM_CMD_PALL; /* Set MODE bits to "010" */
    _command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    _command.AutoRefreshNumber         = 1;
    _command.ModeRegisterDefinition    = 0;
    if(HAL_SDRAM_SendCommand(&_hsdram1, &_command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 6: Configure an Auto Refresh command */
    _command.CommandMode               = FMC_SDRAM_CMD_AUTOREFRESH_MODE; /* Set MODE bits to "011" */
    _command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    _command.AutoRefreshNumber         = 8;
    _command.ModeRegisterDefinition   = 0;
    if (HAL_SDRAM_SendCommand(&_hsdram1, &_command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 7: Program the external memory mode register */
    _modeRegister                     = (0 << 0) | (0 << 2) | (2 << 4) | (0 << 7) | (1 << 9);
    _command.CommandMode               = FMC_SDRAM_CMD_LOAD_MODE;
    _command.ModeRegisterDefinition    = _modeRegister;
    _command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    _command.AutoRefreshNumber        = 1;
    HAL_SDRAM_SendCommand(&_hsdram1, &_command, 0xfff);

    /* Step 8: Set the refresh rate counter - refer to section SDRAM refresh timer register in RM0455 */
    /* Set the device refresh rate
    * COUNT = [(SDRAM self refresh time / number of row) x  SDRAM CLK] – 20
            = [(64ms/8192) * 133.3333MHz] - 20 = 1021.6667 */
    if (HAL_SDRAM_ProgramRefreshRate(&_hsdram1, 1022) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief   Initializaton of the FMC Hardware (PLL, GPIOs, Clocks etc).
 * 
 */
static void HAL_FMC_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct ={0};
    if (_stm32FmcInitialized)
    {
        return;
    }
    _stm32FmcInitialized = 1;
    
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Initializes the peripherals clock
     */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;
        PeriphClkInitStruct.PLL2.PLL2M = 6;
        PeriphClkInitStruct.PLL2.PLL2N = 200;
        PeriphClkInitStruct.PLL2.PLL2P = 2;
        PeriphClkInitStruct.PLL2.PLL2Q = 2;
        PeriphClkInitStruct.PLL2.PLL2R = 2;
        PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
        PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
        PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
        Error_Handler();
        }

    /* Peripheral clock enable */
    __HAL_RCC_FMC_CLK_ENABLE();

    /** FMC GPIO Configuration
     PF0   ------> FMC_A0
    PF1   ------> FMC_A1
    PF2   ------> FMC_A2
    PF3   ------> FMC_A3
    PF4   ------> FMC_A4
    PF5   ------> FMC_A5
    PC0   ------> FMC_SDNWE
    PF11   ------> FMC_SDNRAS
    PF12   ------> FMC_A6
    PF13   ------> FMC_A7
    PF14   ------> FMC_A8
    PF15   ------> FMC_A9
    PG0   ------> FMC_A10
    PG1   ------> FMC_A11
    PE7   ------> FMC_D4
    PE8   ------> FMC_D5
    PE9   ------> FMC_D6
    PE10   ------> FMC_D7
    PE11   ------> FMC_D8
    PE12   ------> FMC_D9
    PE13   ------> FMC_D10
    PE14   ------> FMC_D11
    PE15   ------> FMC_D12
    PD8   ------> FMC_D13
    PD9   ------> FMC_D14
    PD10   ------> FMC_D15
    PD14   ------> FMC_D0
    PD15   ------> FMC_D1
    PG2   ------> FMC_A12
    PG4   ------> FMC_BA0
    PG5   ------> FMC_BA1
    PG8   ------> FMC_SDCLK
    PD0   ------> FMC_D2
    PD1   ------> FMC_D3
    PD4   ------> FMC_NOE
    PD5   ------> FMC_NWE
    PG10   ------> FMC_NE3
    PG15   ------> FMC_SDNCAS
    PB5   ------> FMC_SDCKE1
    PB6   ------> FMC_SDNE1
    PE0   ------> FMC_NBL0
    PE1   ------> FMC_NBL1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                            |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11|GPIO_PIN_12
                            |GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                            |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                            |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                            |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
                            |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4
                            |GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief   STM32 HAL function for initialization STM32 FMC peripheral (ePaper peripheral).
 * 
 */
extern "C" void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
    HAL_FMC_MspInit();
}

/**
 * @brief   Deinitializaton of the FMC Hardware (PLL, GPIOs, Clocks etc).
 * 
 */
static void HAL_FMC_MspDeInit(void)
{
    if (_stm32FmcDeInitialized)
    {
        return;
    }
    _stm32FmcDeInitialized = 1;

    /* Peripheral clock enable */
    __HAL_RCC_FMC_CLK_DISABLE();

    /** FMC GPIO Configuration
     PF0   ------> FMC_A0
    PF1   ------> FMC_A1
    PF2   ------> FMC_A2
    PF3   ------> FMC_A3
    PF4   ------> FMC_A4
    PF5   ------> FMC_A5
    PC0   ------> FMC_SDNWE
    PF11   ------> FMC_SDNRAS
    PF12   ------> FMC_A6
    PF13   ------> FMC_A7
    PF14   ------> FMC_A8
    PF15   ------> FMC_A9
    PG0   ------> FMC_A10
    PG1   ------> FMC_A11
    PE7   ------> FMC_D4
    PE8   ------> FMC_D5
    PE9   ------> FMC_D6
    PE10   ------> FMC_D7
    PE11   ------> FMC_D8
    PE12   ------> FMC_D9
    PE13   ------> FMC_D10
    PE14   ------> FMC_D11
    PE15   ------> FMC_D12
    PD8   ------> FMC_D13
    PD9   ------> FMC_D14
    PD10   ------> FMC_D15
    PD14   ------> FMC_D0
    PD15   ------> FMC_D1
    PG2   ------> FMC_A12
    PG4   ------> FMC_BA0
    PG5   ------> FMC_BA1
    PG8   ------> FMC_SDCLK
    PD0   ------> FMC_D2
    PD1   ------> FMC_D3
    PD4   ------> FMC_NOE
    PD5   ------> FMC_NWE
    PG10   ------> FMC_NE3
    PG15   ------> FMC_SDNCAS
    PB5   ------> FMC_SDCKE1
    PB6   ------> FMC_SDNE1
    PE0   ------> FMC_NBL0
    PE1   ------> FMC_NBL1
    */
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                            |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11|GPIO_PIN_12
                            |GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                            |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                            |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                            |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
                            |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4
                            |GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5|GPIO_PIN_6);
}

/**
 * @brief   STM32 HAL function for deinitialization STM32 FMC peripheral (ePaper peripheral).
 * 
 */
extern "C" void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef *hsram)
{
    HAL_FMC_MspDeInit();
}

/**
 * @brief   STM32 HAL function for deinitialization STM32 FMC peripheral (SDRAM peripheral).
 * 
 */
extern "C" void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram)
{
    HAL_FMC_MspDeInit();
}

/**
 * @brief   Function sets and initializes whole STM32 FMC peripheral.
 * 
 * @param   uint32_t _ePaperPeriphAddress
 *          SRAM LCD (ePaper) peripheral address - used to disable cache on this address.
 */
void stm32FmcInit(uint32_t _ePaperPeriphAddress)
{
    INKPLATE_DEBUG_MGS("STM32 FMC Driver Init started");

    // Enable clock to GPIOs
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // Enable clock to FMC
    __HAL_RCC_FMC_CLK_ENABLE();

    // Init FMC
    MX_FMC_Init();

    /* L1 cache issue. On Atollic, STM32 was sending extra bytes (8 instead 1), on Arduino FMC did not work at all!
     * https://community.st.com/s/question/0D50X0000C9hD8D/stm32f746-fmc-configured-for-8bit-parallel-interface-extra-bytes-sent
     * https://community.st.com/s/question/0D50X00009XkWQE/stm32h743ii-fmc-8080-lcd-spurious-writes
     * https://stackoverflow.com/questions/59198934/l1-cache-behaviour-of-stm32h7
     *
     * It can be fixed by enabling HAL_SetFMCMemorySwappingConfig(FMC_SWAPBMAP_SDRAM_SRAM); but this will hurt SDRAM R/W
     * performace! Real workaround is to disable cache on LCD memory allocation with MPU (Memory Protection Unit).
     */
    stm32FmcMpuInit(_ePaperPeriphAddress);

    // Init DMA (MDMA - Master DMA) for external RAM.
    stm32FmcMdmaInit();

    // Link FMC and Master DMA for RAM.
    _hsdram1.hmdma = &_hmdmaMdmaChannel40Sw0;

    // Link FMC and Master DMA for EPD.
    _hsram1.hmdma = &_hmdmaMdmaChannel41Sw0;

    // Create DMA Transfer callbacks.
    HAL_MDMA_RegisterCallback(&_hmdmaMdmaChannel40Sw0, HAL_MDMA_XFER_CPLT_CB_ID, stm32FmcSdramTransferCompleteCallback);
    HAL_MDMA_RegisterCallback(&_hmdmaMdmaChannel41Sw0, HAL_MDMA_XFER_CPLT_CB_ID, stm32FmcEpdTransferCompleteCallback);

    INKPLATE_DEBUG_MGS("STM32 FMC Driver Init done");
}

/**
 * @brief   Deinitializaton of the complete FMC peripheral.
 * 
 */
void stm32FmcDeInit()
{
    // De-Init FMC.
    HAL_FMC_MspDeInit();

    // Disable clock to FMC.
    __HAL_RCC_FMC_CLK_ENABLE();
}

/**
 * @brief   Initializaton of the STM32 MDMA (Master Direct Memory Access controller).
 *          Used for getting data from the SDRAM into internal SRAM and to transfer
 *          ePaper dat ato the ePaper fast as possible. Also enables the interrutps on DMA.
 * 
 */
void stm32FmcMdmaInit()
{
    /* MDMA controller clock enable */
    __HAL_RCC_MDMA_CLK_ENABLE();
    /* Local variables */

    /* Configure MDMA channel MDMA_Channel0 */
    /* Configure MDMA request hmdmaMdmaChannel40Sw0 on MDMA_Channel0 */
    _hmdmaMdmaChannel40Sw0.Instance = MDMA_Channel0;
    _hmdmaMdmaChannel40Sw0.Init.Request = MDMA_REQUEST_SW;
    _hmdmaMdmaChannel40Sw0.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
    _hmdmaMdmaChannel40Sw0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    _hmdmaMdmaChannel40Sw0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    _hmdmaMdmaChannel40Sw0.Init.SourceInc = MDMA_SRC_INC_WORD;
    _hmdmaMdmaChannel40Sw0.Init.DestinationInc = MDMA_DEST_INC_WORD;
    _hmdmaMdmaChannel40Sw0.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
    _hmdmaMdmaChannel40Sw0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
    _hmdmaMdmaChannel40Sw0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    _hmdmaMdmaChannel40Sw0.Init.BufferTransferLength = 128;
    _hmdmaMdmaChannel40Sw0.Init.SourceBurst = MDMA_SOURCE_BURST_128BEATS;
    _hmdmaMdmaChannel40Sw0.Init.DestBurst = MDMA_DEST_BURST_128BEATS;
    _hmdmaMdmaChannel40Sw0.Init.SourceBlockAddressOffset = 0;
    _hmdmaMdmaChannel40Sw0.Init.DestBlockAddressOffset = 0;
    if (HAL_MDMA_Init(&_hmdmaMdmaChannel40Sw0) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure MDMA channel MDMA_Channel1 */
    /* Configure MDMA request hmdma_mdma_channel41_sw_0 on MDMA_Channel1 */
    _hmdmaMdmaChannel41Sw0.Instance = MDMA_Channel1;
    _hmdmaMdmaChannel41Sw0.Init.Request = MDMA_REQUEST_SW;
    _hmdmaMdmaChannel41Sw0.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
    _hmdmaMdmaChannel41Sw0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    _hmdmaMdmaChannel41Sw0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    _hmdmaMdmaChannel41Sw0.Init.SourceInc = MDMA_SRC_INC_BYTE;
    _hmdmaMdmaChannel41Sw0.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
    _hmdmaMdmaChannel41Sw0.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
    _hmdmaMdmaChannel41Sw0.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
    _hmdmaMdmaChannel41Sw0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    _hmdmaMdmaChannel41Sw0.Init.BufferTransferLength = 128;
    _hmdmaMdmaChannel41Sw0.Init.SourceBurst = MDMA_SOURCE_BURST_128BEATS;
    _hmdmaMdmaChannel41Sw0.Init.DestBurst = MDMA_DEST_BURST_128BEATS;
    _hmdmaMdmaChannel41Sw0.Init.SourceBlockAddressOffset = 0;
    _hmdmaMdmaChannel41Sw0.Init.DestBlockAddressOffset = 0;
    if (HAL_MDMA_Init(&_hmdmaMdmaChannel41Sw0) != HAL_OK)
    {
      Error_Handler();
    }

    HAL_NVIC_SetPriority(MDMA_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(MDMA_IRQn);
}

/**
 * @brief   Returns the address of the SRAM instance (used for data transfer between STM32 and ePaper).
 * 
 * @return  SRAM_HandleTypeDef*
 *          Pointer to the STM32 SRAM/LCD FMC Instance.
 */
SRAM_HandleTypeDef *stm32FmcGetEpdInstance()
{
    // Handle for the FMC LCD interface (for EPD).
    return &_hsram1;
}

/**
 * @brief   Returns the address of the SDRAM instance (used for communication between SDRAM and STM32).
 * 
 * @return  SDRAM_HandleTypeDef*
 *          Pointer to the STM32 SDRAM FMC Instance.
 */
SDRAM_HandleTypeDef *stm32FmcGetSdramInstance()
{
    // Handle for the SDRAM FMC interface.
    return &_hsdram1;
}

/**
 * @brief   Returns the STM32 Master DMA instance used by the ePaper (STM32->ePaper data transfer).
 * 
 * @return  MDMA_HandleTypeDef*
 *          Address of the Master DMA STM32 instance.
 */
MDMA_HandleTypeDef *stm32FmcGetEpdMdmaInstance()
{
    // Handle for Master DMA for FMC LCD (EPD).
    return &_hmdmaMdmaChannel41Sw0;
}

// 
/**
 * @brief   Returns the STM32 Master DMA instance used by the SDRAM (STM32<-SDRAM data transfer).
 * 
 * @return  MDMA_HandleTypeDef*
 *          Address of the Master DMA STM32 instance.
 * 
 * @note    It is used for data transfer between SDRAM and STM32, but not otherway around.
 */
MDMA_HandleTypeDef *stm32FmcGetSdramMdmaInstance()
{
    // Handle for Master DMA for SDRAM.
    return &_hmdmaMdmaChannel40Sw0;
}

/**
 * @brief   Gets the instance of the STM32 MPU (Memory Protection Unit).
 * 
 * @return  MPU_Region_InitTypeDef*
 *          Returns the address of the STM32 MPU instance.
 */
MPU_Region_InitTypeDef *stm32FmcGetMpuInstance()
{
    return &_mpuInitStructEpd;
}

/**
 * @brief   It disables cacheing on LCD FMC memory area, but not affecting caching on SRAM by using MPU.
 *          Initializaton and setup of the MPU on FMC SRAM part of the peripheral. This is neeeded due epaper
 *          control timings.
 * 
 * @param   uint32_t _ePaperPeriphAddress
 *          SRAM LCD (ePaper) peripheral address - used to disable cache on this address.
 */
void stm32FmcMpuInit(uint32_t _epaperPeriphAddress)
{
    INKPLATE_DEBUG_MGS("STM32 MPU Init started");

    HAL_MPU_Disable();
    // Disable only cache on LCD interface, NOT SRAM!
    _mpuInitStructEpd.Enable = MPU_REGION_ENABLE;
    _mpuInitStructEpd.BaseAddress = _epaperPeriphAddress;
    _mpuInitStructEpd.Size = MPU_REGION_SIZE_64MB;
    _mpuInitStructEpd.AccessPermission = MPU_REGION_FULL_ACCESS;
    _mpuInitStructEpd.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    _mpuInitStructEpd.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    _mpuInitStructEpd.IsShareable = MPU_ACCESS_SHAREABLE;
    _mpuInitStructEpd.Number = MPU_REGION_NUMBER1;
    _mpuInitStructEpd.TypeExtField = MPU_TEX_LEVEL0;
    _mpuInitStructEpd.SubRegionDisable = 0x00;
    _mpuInitStructEpd.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&_mpuInitStructEpd);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    INKPLATE_DEBUG_MGS("STM32 MPU Init done");
}

/**
 * @brief   Callback function called after the data transfer for the SDRAM has completed.
 * 
 * @param   MDMA_HandleTypeDef *_mdma
 *          Pointer to the MDMA handle - required by the STM32 HAL library.
 */
void stm32FmcSdramTransferCompleteCallback(MDMA_HandleTypeDef *_mdma)
{
    _stm32MdmaSdramCompleteFlag = 1;
}

/**
 * @brief   Callback function called after the data transfer for the ePaper has completed.
 * 
 * @param   MDMA_HandleTypeDef *_mdma
 *          Pointer to the MDMA handle - required by the STM32 HAL library.
 */
void stm32FmcEpdTransferCompleteCallback(MDMA_HandleTypeDef *_mdma)
{
    _stm32MdmaEpdCompleteFlag = 1;
}

/**
 * @brief   Clears the transfer complete flag to ready for the next transfer.
 * 
 */
void stm32FmcClearEpdCompleteFlag()
{
    _stm32MdmaEpdCompleteFlag = 0;
}

/**
 * @brief   Clears the transfer complete flag to ready for the next transfer.
 * 
 */
void stm32FmcClearSdramCompleteFlag()
{
    _stm32MdmaSdramCompleteFlag = 0;
}

/**
 * @brief   Returns the transfer complete flag state.
 * 
 * @return  uint8_t
 *          1 = transfer complete.
 *          0 = transfet still in progress.
 * 
 */
uint8_t stm32FmcEpdCompleteFlag()
{
    return _stm32MdmaEpdCompleteFlag;
}

/**
 * @brief   Returns the transfer complete flag state.
 * 
 * @return  uint8_t
 *          1 = transfer complete.
 *          0 = transfet still in progress.
 * 
 */
uint8_t stm32FmcSdramCompleteFlag()
{
    return _stm32MdmaSdramCompleteFlag;
}

/**
 * @brief   STM32 function for interrupt callback register.
 * 
 */
extern "C" void MDMA_IRQHandler()
{
    HAL_MDMA_IRQHandler(&_hmdmaMdmaChannel40Sw0);
    HAL_MDMA_IRQHandler(&_hmdmaMdmaChannel41Sw0);
}