// Include header file of this .cpp file
#include "stm32FMC.h"

// FMC HAL Typedefs and init status variables.
// Handle for the FMC LCD interface (for EPD).
SRAM_HandleTypeDef hsram1;
// Handle for the SDRAM FMC interface.
SDRAM_HandleTypeDef hsdram1;
// Handle for Master DMA for SDRAM.
MDMA_HandleTypeDef hmdma_mdma_channel40_sw_0;
// Handle for Master DMA for FMC LCD (EPD).
MDMA_HandleTypeDef hmdma_mdma_channel41_sw_0;
// Handle memory protection unit.
MPU_Region_InitTypeDef MPU_InitStructEPD;
static uint32_t FMC_Initialized = 0;
static uint32_t FMC_DeInitialized = 0;

uint8_t _stm32MdmaEPDCompleteFlag = 0;
uint8_t _stm32MdmaSRAMCompleteFlag = 0;

// Really low level STM32 related stuff. Do not change anything unless you really know what you are doing!
/* FMC initialization function */
static void MX_FMC_Init(void)
{
    FMC_NORSRAM_TimingTypeDef Timing = {0};
    FMC_SDRAM_TimingTypeDef SdramTiming = {0};

    /** Perform the SRAM1 memory initialization sequence
     */
    hsram1.Instance = FMC_NORSRAM_DEVICE;
    hsram1.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram1.Init */
    hsram1.Init.NSBank = FMC_NORSRAM_BANK3;
    hsram1.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
    hsram1.Init.MemoryType = FMC_MEMORY_TYPE_SRAM;
    hsram1.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_8;
    hsram1.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_DISABLE;
    hsram1.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram1.Init.WaitSignalActive = FMC_WAIT_TIMING_BEFORE_WS;
    hsram1.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
    hsram1.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
    hsram1.Init.ExtendedMode = FMC_EXTENDED_MODE_DISABLE;
    hsram1.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram1.Init.WriteBurst = FMC_WRITE_BURST_DISABLE;
    hsram1.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ONLY;
    hsram1.Init.WriteFifo = FMC_WRITE_FIFO_DISABLE;
    hsram1.Init.PageSize = FMC_PAGE_SIZE_NONE;
    /* Timing */
    Timing.AddressSetupTime = 0;
    Timing.AddressHoldTime = 0;
    Timing.DataSetupTime = 2;
    Timing.BusTurnAroundDuration = 0;
    Timing.CLKDivision = 1;
    Timing.DataLatency = 1;
    Timing.AccessMode = FMC_ACCESS_MODE_A;
    /* ExtTiming */

    if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
    {
        Error_Handler( );
    }

    /** Perform the SDRAM1 memory initialization sequence
     */
    hsdram1.Instance = FMC_SDRAM_DEVICE;
    /* hsdram1.Init */
    hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
    hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
    hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
    hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
    hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
    /* SdramTiming */
    SdramTiming.LoadToActiveDelay = 2;
    SdramTiming.ExitSelfRefreshDelay = 10;
    SdramTiming.SelfRefreshTime = 2;
    SdramTiming.RowCycleDelay = 8;
    SdramTiming.WriteRecoveryTime = 4;
    SdramTiming.RPDelay = 2;
    SdramTiming.RCDDelay = 2;

    if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
    {
        Error_Handler( );
    }

    __IO uint32_t tmpmrd              = 0;
    FMC_SDRAM_CommandTypeDef Command  = {0};

    Command.CommandMode               = FMC_SDRAM_CMD_CLK_ENABLE; /* Set MODE bits to "001" */
    Command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2; /* configure the Target Bank bits */
    Command.AutoRefreshNumber         = 1;
    Command.ModeRegisterDefinition    = 0;
    if (HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_Delay(1); /* Step 4: Insert 100 us minimum delay - Min HAL Delay is 1ms */

    /* Step 5: Configure a PALL (precharge all) command */
    Command.CommandMode               = FMC_SDRAM_CMD_PALL; /* Set MODE bits to "010" */
    Command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber         = 1;
    Command.ModeRegisterDefinition    = 0;
    if(HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 6: Configure an Auto Refresh command */
    Command.CommandMode               = FMC_SDRAM_CMD_AUTOREFRESH_MODE; /* Set MODE bits to "011" */
    Command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber         = 8;
    Command.ModeRegisterDefinition   = 0;
    if (HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xfff) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 7: Program the external memory mode register */
    tmpmrd                            = (0 << 0) | (0 << 2) | (2 << 4) | (0 << 7) | (1 << 9);
    Command.CommandMode               = FMC_SDRAM_CMD_LOAD_MODE;
    Command.ModeRegisterDefinition    = tmpmrd;
    Command.CommandTarget             = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber        = 1;
    HAL_SDRAM_SendCommand(&hsdram1, &Command, 0xfff);

    /* Step 8: Set the refresh rate counter - refer to section SDRAM refresh timer register in RM0455 */
    /* Set the device refresh rate
    * COUNT = [(SDRAM self refresh time / number of row) x  SDRAM CLK] – 20
            = [(64ms/8192) * 133.3333MHz] - 20 = 1021.6667 */
    if (HAL_SDRAM_ProgramRefreshRate(&hsdram1, 1022) != HAL_OK)
    {
        Error_Handler();
    }
}

static void HAL_FMC_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct ={0};
    if (FMC_Initialized)
    {
        return;
    }
    FMC_Initialized = 1;
    
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

extern "C" void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
    HAL_FMC_MspInit();
}

static void HAL_FMC_MspDeInit(void)
{
    if (FMC_DeInitialized)
    {
        return;
    }
    FMC_DeInitialized = 1;

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

extern "C" void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef *hsram)
{
    HAL_FMC_MspDeInit();
}

extern "C" void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram)
{
    HAL_FMC_MspDeInit();
}

void stm32FmcInit()
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
     * It can be fixed by enabling HAL_SetFMCMemorySwappingConfig(FMC_SWAPBMAP_SDRAM_SRAM); but this will hurt SRAM R/W
     * performace! Real workaround is to disable cache on LCD memory allocation with MPU (Memory Protection Unit).
     */
    stm32MpuInit();

    // Init DMA (MDMA - Master DMA) for external RAM.
    stm32MDMAInit();

    // Link FMC and Master DMA for RAM.
    hsdram1.hmdma = &hmdma_mdma_channel40_sw_0;

    // Link FMC and Master DMA for EPD.
    hsram1.hmdma = &hmdma_mdma_channel41_sw_0;

    // Create DMA Transfer callbacks.
    HAL_MDMA_RegisterCallback(&hmdma_mdma_channel40_sw_0, HAL_MDMA_XFER_CPLT_CB_ID, stm32FMCSRAMTransferCompleteCallback);
    HAL_MDMA_RegisterCallback(&hmdma_mdma_channel41_sw_0, HAL_MDMA_XFER_CPLT_CB_ID, stm32FMCEPDTransferCompleteCallback);

    INKPLATE_DEBUG_MGS("STM32 FMC Driver Init done");
}

void stm32MDMAInit()
{
    /* MDMA controller clock enable */
    __HAL_RCC_MDMA_CLK_ENABLE();
    /* Local variables */

    /* Configure MDMA channel MDMA_Channel0 */
    /* Configure MDMA request hmdma_mdma_channel40_sw_0 on MDMA_Channel0 */
    hmdma_mdma_channel40_sw_0.Instance = MDMA_Channel0;
    hmdma_mdma_channel40_sw_0.Init.Request = MDMA_REQUEST_SW;
    hmdma_mdma_channel40_sw_0.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
    hmdma_mdma_channel40_sw_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    hmdma_mdma_channel40_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    hmdma_mdma_channel40_sw_0.Init.SourceInc = MDMA_SRC_INC_WORD;
    hmdma_mdma_channel40_sw_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
    hmdma_mdma_channel40_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
    hmdma_mdma_channel40_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
    hmdma_mdma_channel40_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    hmdma_mdma_channel40_sw_0.Init.BufferTransferLength = 128;
    hmdma_mdma_channel40_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_128BEATS;
    hmdma_mdma_channel40_sw_0.Init.DestBurst = MDMA_DEST_BURST_128BEATS;
    hmdma_mdma_channel40_sw_0.Init.SourceBlockAddressOffset = 0;
    hmdma_mdma_channel40_sw_0.Init.DestBlockAddressOffset = 0;
    if (HAL_MDMA_Init(&hmdma_mdma_channel40_sw_0) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure MDMA channel MDMA_Channel1 */
    /* Configure MDMA request hmdma_mdma_channel41_sw_0 on MDMA_Channel1 */
    hmdma_mdma_channel41_sw_0.Instance = MDMA_Channel1;
    hmdma_mdma_channel41_sw_0.Init.Request = MDMA_REQUEST_SW;
    hmdma_mdma_channel41_sw_0.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
    hmdma_mdma_channel41_sw_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    hmdma_mdma_channel41_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    hmdma_mdma_channel41_sw_0.Init.SourceInc = MDMA_SRC_INC_BYTE;
    hmdma_mdma_channel41_sw_0.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
    hmdma_mdma_channel41_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
    hmdma_mdma_channel41_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
    hmdma_mdma_channel41_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    hmdma_mdma_channel41_sw_0.Init.BufferTransferLength = 128;
    hmdma_mdma_channel41_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_128BEATS;
    hmdma_mdma_channel41_sw_0.Init.DestBurst = MDMA_DEST_BURST_128BEATS;
    hmdma_mdma_channel41_sw_0.Init.SourceBlockAddressOffset = 0;
    hmdma_mdma_channel41_sw_0.Init.DestBlockAddressOffset = 0;
    if (HAL_MDMA_Init(&hmdma_mdma_channel41_sw_0) != HAL_OK)
    {
      Error_Handler();
    }

    HAL_NVIC_SetPriority(MDMA_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(MDMA_IRQn);
}

/**
 * @brief       It disables cacheing on LCD FMC memory area, but not affecting caching on SRAM by using MPU.
 *
 */
void stm32MpuInit()
{
    INKPLATE_DEBUG_MGS("STM32 MPU Init started");

    HAL_MPU_Disable();
    // Disable only cache on LCD interface, NOT SRAM!
    MPU_InitStructEPD.Enable = MPU_REGION_ENABLE;
    MPU_InitStructEPD.BaseAddress = 0x68000000;
    MPU_InitStructEPD.Size = MPU_REGION_SIZE_64MB;
    MPU_InitStructEPD.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStructEPD.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStructEPD.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStructEPD.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStructEPD.Number = MPU_REGION_NUMBER1;
    MPU_InitStructEPD.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStructEPD.SubRegionDisable = 0x00;
    MPU_InitStructEPD.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStructEPD);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    INKPLATE_DEBUG_MGS("STM32 MPU Init done");
}

void stm32FMCSRAMTransferCompleteCallback(MDMA_HandleTypeDef *_mdma)
{
    _stm32MdmaSRAMCompleteFlag = 1;
}

void stm32FMCEPDTransferCompleteCallback(MDMA_HandleTypeDef *_mdma)
{
    _stm32MdmaEPDCompleteFlag = 1;
}

void stm32FMCClearEPDCompleteFlag()
{
    _stm32MdmaEPDCompleteFlag = 0;
}

void stm32FMCClearSRAMCompleteFlag()
{
    _stm32MdmaSRAMCompleteFlag = 0;
}

uint8_t stm32FMCEPDCompleteFlag()
{
    return _stm32MdmaEPDCompleteFlag;
}

uint8_t stm32FMCSRAMCompleteFlag()
{
    return _stm32MdmaSRAMCompleteFlag;
}

extern "C" void MDMA_IRQHandler()
{
    HAL_MDMA_IRQHandler(&hmdma_mdma_channel40_sw_0);
    HAL_MDMA_IRQHandler(&hmdma_mdma_channel41_sw_0);
}