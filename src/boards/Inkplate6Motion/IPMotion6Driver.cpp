#include "IPMotion6Driver.h"

// Header guard for the Arduino include
#ifdef BOARD_INKPLATE6_MOTION

// Macro function to round array size to be multiple of 4 (needed for DMA controller, since it works with 32 bit
// values).
#define MULTIPLE_OF_4(x) (((x - 1) | 3) + 1)

// Buffer for one Line on the screen from epapper framebuffer from exterenal RAM. It's packed 4 bits per pixel.
__attribute__((section(".dma_buffer"))) uint8_t _oneLine1[MULTIPLE_OF_4(SCREEN_WIDTH / 2 * 16)];
__attribute__((section(".dma_buffer"))) uint8_t _oneLine2[MULTIPLE_OF_4(SCREEN_WIDTH / 2 * 16)];
__attribute__((section(".dma_buffer"))) uint8_t _oneLine3[MULTIPLE_OF_4(SCREEN_WIDTH / 2 * 32)];
// Buffer for decoded pixels modified by the EPD waveform. EPD uses 4 pixels per byte (2 bits per pixel).
__attribute__((section(".dma_buffer"))) uint8_t _decodedLine1[MULTIPLE_OF_4(SCREEN_WIDTH / 4) + 2];
__attribute__((section(".dma_buffer"))) uint8_t _decodedLine2[MULTIPLE_OF_4(SCREEN_WIDTH / 4) + 2];
// Pointer to the decoded line buffers.
__attribute__((section(".dma_buffer"))) uint8_t *_currentDecodedLineBuffer = NULL;
__attribute__((section(".dma_buffer"))) uint8_t *_pendingDecodedLineBuffer = NULL;

extern SRAM_HandleTypeDef hsram1;   // EPD
extern SDRAM_HandleTypeDef hsdram1; // SDRAM
extern MDMA_HandleTypeDef hmdma_mdma_channel40_sw_0;
extern MDMA_HandleTypeDef hmdma_mdma_channel41_sw_0;

EPDDriver::EPDDriver()
{
    // Empty constructor.
}

int EPDDriver::initDriver()
{
    INKPLATE_DEBUG_MGS("EPD Driver init started");

    // Configure IO expander.
    if (!internalIO.beginIO(IO_EXPANDER_INTERNAL_I2C_ADDR))
    {
        INKPLATE_DEBUG_MGS("GPIO expander init fail");
    }

    // Configure GPIO pins.
    gpioInit();

    // Enable TPS65186 and keep it on.
    internalIO.digitalWriteIO(TPS_WAKE_PIN, HIGH, true);

    // Wait a little bit for PMIC.
    delay(10);

    // Init PMIC. Return error if failed.
    if (!pmic.begin())
    {
        INKPLATE_DEBUG_MGS("EPD PMIC init failed!");
        return 0;
    }

    // Init STM32 FMC (Flexible memory controller) for faster pushing data to panel using Hardware (similar to ESP32
    // I2S). Parallel, but much faster and better).
    stm32FmcInit();

    // Send proper power up sequence for EPD.
    pmic.setPowerOffSeq(0b00000000, 0b00000000);

    // Turn off EPD PMIC.
    internalIO.digitalWriteIO(TPS_WAKE_PIN, LOW, true);

    // Initialize microSD driver. Actually it's just kinda wrapper for microSD class.
    microSD.begin(&_systemSpi, INKPLATE_MICROSD_SPI_CS, 20);

    INKPLATE_DEBUG_MGS("EPD Driver init done");

    // Everything went ok? Return 1 for success.
    return 1;
}

void EPDDriver::cleanFast(uint8_t *_clearWavefrom, uint8_t _wavefromPhases)
{
    // Enable EPD PSU.
    epdPSU(1);

    for (int k = 0; k < _wavefromPhases; k++)
    {
        // Convert EPD wavefrom byte to EPD Data.
        uint8_t _data = wavefromElementToEpdData(_clearWavefrom[k]);

        // Fill the buffer with the color.
        for (int i = 0; i < (sizeof(_decodedLine1)); i++)
        {
            _decodedLine1[i] = _data;
        }

        // Start a new frame.
        vScanStart();

        // Push data to all rows.
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            // Start vertical scan.
            hScanStart(_data, _data);

            // Start DMA transfer!
            HAL_MDMA_Start_IT(&hmdma_mdma_channel41_sw_0, (uint32_t)_decodedLine1, (uint32_t)EPD_FMC_ADDR,
                              sizeof(_decodedLine1) - 2, 1);

            // Wait until the transfer has ended.
            while (!stm32FMCEPDCompleteFlag())
                ;

            // Clear the flag.
            stm32FMCClearEPDCompleteFlag();

            // End the line write.
            vScanEnd();
        }
    }

    // EPD PSU won't be turned off here after update.
    // It needs to be additionally or manually turned off.
}

void EPDDriver::clearDisplay()
{
    // Framebuffer if filled with different data depending on the cuurrent mode.
    // If the
    if (getDisplayMode() == INKPLATE_1BW)
    {
        for (int i = 0; i < (SCREEN_HEIGHT * SCREEN_WIDTH / 8); i++)
        {
            _pendingScreenFB[i] = 0;
        }
    }

    if (getDisplayMode() == INKPLATE_GL16)
    {
        for (int i = 0; i < (SCREEN_HEIGHT * SCREEN_WIDTH / 2); i++)
        {
            _pendingScreenFB[i] = 255;
        }
    }
}

void EPDDriver::partialUpdate(uint8_t _leaveOn)
{
    INKPLATE_DEBUG_MGS("Partial update 1bit start");

    // Check if the Inkplate library is in correct display mode.
    if (getDisplayMode() != INKPLATE_1BW)
        return;


    // Check if there is already one full update. If notm skip partial update and force full update.
    if (_blockPartial == 1)
    {
        display1b(_leaveOn);
        return;
    }

    // Check if automatic full update is enabled. If so, check if the full update needs to be executed.
    if (_partialUpdateLimiter != 0)
    {
        if (_partialUpdateCounter >= _partialUpdateLimiter)
        {
            // Force full update.
            display1b(_leaveOn);

            // Reset the counter!
            _partialUpdateCounter = 0;

            // Go back!
            return;
        }
    }

    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Pointer to the framebuffer (used by the fast GLUT). It gets 4 pixels from the framebuffer.
    uint8_t *_fbPtr;

    // Find the difference mask for the partial update (use scratchpad memory!).
    uint32_t _pxDiff = differenceMask((uint8_t *)_currentScreenFB, (uint8_t *)_pendingScreenFB, (uint8_t *)_scratchpadMemory);

    for (int k = 0; k < 9; k++)
    {
        volatile uint8_t *ptr = _scratchpadMemory;

        // Get the 32 rows of the data (faster RAM read speed, since it reads whole RAM column at once).
        // Reading line by line will gets us only 89MB/s read speed, but reading 32 rows or more at once will get us ~215MB/s read speed! Nice!
        // Start the DMA transfer!
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_scratchpadMemory, (uint32_t)_oneLine1, sizeof(_oneLine1), 1);
        while(stm32FMCSRAMCompleteFlag() == 0);
        stm32FMCClearSRAMCompleteFlag();
        ptr += sizeof(_oneLine1);

        // Set the current working RAM buffer to the first RAM Buffer (_oneLine1).
        _fbPtr = (uint8_t*)_oneLine1;

        // Copy data of the first line into the buffer for the EPD.
        memcpy(_decodedLine1, _fbPtr, (SCREEN_WIDTH / 4));
        _fbPtr += (SCREEN_WIDTH / 4);

        // Set the pointers for double buffering.
        _pendingDecodedLineBuffer = _decodedLine2;
        _currentDecodedLineBuffer = _decodedLine1;

        // Send to the screen!
        vScanStart();
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            hScanStart(_currentDecodedLineBuffer[0], _currentDecodedLineBuffer[1]);
            HAL_MDMA_Start_IT(&hmdma_mdma_channel41_sw_0, (uint32_t)_currentDecodedLineBuffer + 2, (uint32_t)EPD_FMC_ADDR, sizeof(_decodedLine1), 1);

            // Copy data into the buffer for the EPD.
            memcpy(_pendingDecodedLineBuffer, _fbPtr, (SCREEN_WIDTH / 4));
            _fbPtr += (SCREEN_WIDTH / 4);

            // Swap the buffers!
            if (_currentDecodedLineBuffer == _decodedLine1)
            {
                _currentDecodedLineBuffer = _decodedLine2;
                _pendingDecodedLineBuffer = _decodedLine1;
            }
            else
            {
                _currentDecodedLineBuffer = _decodedLine1;
                _pendingDecodedLineBuffer = _decodedLine2;
            }

            // Can't start new transfer until all data is sent to EPD.
            while(stm32FMCEPDCompleteFlag() == 0);
            stm32FMCClearEPDCompleteFlag();

            // Advance the line on EPD.
            vScanEnd();

            // Check if the buffer needs to be updated (after 32 lines).
            if ((i & 0b00011111) == 0b00011110)
            {
                // Update the buffer pointer.
                _fbPtr = (uint8_t*)_oneLine1;

                // Start new RAM DMA transfer.
                HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)ptr, (uint32_t)(_oneLine1), sizeof(_oneLine1), 1);

                ptr += sizeof(_oneLine1);

                // Wait for DMA transfer to complete.
                while(stm32FMCSRAMCompleteFlag() == 0);
                stm32FMCClearSRAMCompleteFlag();
            }
        }
    }

    // Discharge the e-paper display.
    uint8_t _discharge = 0;
    cleanFast(&_discharge, 1);

    // Copy everything in current screen framebuffer.
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(&hmdma_mdma_channel40_sw_0, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB, (SCREEN_WIDTH * SCREEN_HEIGHT / 8));

    INKPLATE_DEBUG_MGS("Partial update done");

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);

    // Check if automatic full update is enabled.
    if (_partialUpdateLimiter != 0)
    {
        // Increment the counter.
        _partialUpdateCounter++;
    }
}

void EPDDriver::partialUpdate4Bit(uint8_t _leaveOn)
{
    // // TODO!!! Do not alloce whole user RAM just to find the difference mask. Do it line by line!
    // // And also check why image is darker on partial update compared to the full grayscale update!
    // // Anyhow, this must be optimised!
    // const uint8_t _cleanWaveform[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    //                                   2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    //                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2};
    // uint8_t _panelMask[SCREEN_HEIGHT * SCREEN_WIDTH / 4];
    // uint8_t *_pPanelMask = _panelMask;
    // memset(_panelMask, 0, sizeof(_panelMask));

    // // We present you the ugliest code ever...needs A LOT of clean up and optimisation!
    // // Create the mask for pixel difference
    // __IO uint8_t *_pPartial = _pendingScreenFB;
    // __IO uint8_t *_pImage = _currentScreenFB;
    // for (int i = 0; i < SCREEN_HEIGHT; i++)
    // {
    //     for (int j = 0; j < (SCREEN_WIDTH / 4); j++) // Now do all that for whole row
    //     {
    //         uint8_t _diff = *_pPartial++ ^ *_pImage++;
    //         if (_diff & (0xf0))
    //             *_pPanelMask |= 0x30;
    //         if (_diff & (0x0f))
    //             *_pPanelMask |= 0xc0;
    //         _diff = *_pPartial++ ^ *_pImage++;
    //         if (_diff & (0xf0))
    //             *_pPanelMask |= 0x03;
    //         if (_diff & (0x0f))
    //             *_pPanelMask |= 0x0c;
    //         _pPanelMask++;
    //     }
    // }

    // // Power up EPD PMIC. Abort update if failed.
    // if (!epdPSU(1)) return;

    // for (int k = 0; k < 60; k++)
    // {
    //     _pPanelMask = _panelMask;
    //     uint8_t _color;

    //     switch (_cleanWaveform[k])
    //     {
    //     case 0:
    //         _color = B10101010;
    //         break;
    //     case 1:
    //         _color = B01010101;
    //         break;
    //     case 2:
    //         _color = B00000000;
    //         break;
    //     case 3:
    //         _color = B11111111;
    //         break;
    //     }

    //     vScanStart();
    //     for (int i = 0; i < SCREEN_HEIGHT; i++)
    //     {
    //         hScanStart(_color & *_pPanelMask++, _color & *_pPanelMask++); // Start sending first pixel byte to panel
    //         for (int j = 0; j < (SCREEN_WIDTH / 8) - 1; j++)                // Now do all that for whole row
    //         {
    //             *(__IO uint8_t *)(EPD_FMC_ADDR) = _color & *_pPanelMask++;
    //             *(__IO uint8_t *)(EPD_FMC_ADDR) = _color & *_pPanelMask++;
    //         }
    //         vScanEnd(); // Write one row to panel
    //     }
    //     delayMicroseconds(230); // Wait 230uS before new frame
    // }

    // for (int k = 0; k < _wfPhases; k++)
    // {
    //     __IO uint8_t *dp = _pendingScreenFB;
    //     _pPanelMask = _panelMask;
    //     vScanStart();
    //     for (int i = 0; i < SCREEN_HEIGHT; i++)
    //     {
    //         hScanStart((GLUTBW[k * 256 + (*(dp++))] | GLUT1[k * 256 + (*(dp++))]),
    //                     (GLUTBW[k * 256 + (*(dp++))] | GLUT1[k * 256 + (*(dp++))]));
    //         for (int j = 0; j < ((SCREEN_WIDTH / 8)) - 1; j++)
    //         {

    //             *(__IO uint8_t *)(EPD_FMC_ADDR) = (GLUTBW[k * 256 + (*(dp++))] | GLUT1[k * 256 + (*(dp++))]);
    //             *(__IO uint8_t *)(EPD_FMC_ADDR) = (GLUTBW[k * 256 + (*(dp++))] | GLUT1[k * 256 + (*(dp++))]);
    //         }
    //         vScanEnd();
    //     }
    //     delayMicroseconds(230);
    // }
    // cleanFast(2, 2);
    // cleanFast(3, 1);

    // // After update, copy differences to screen buffer
    // for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT / 2); i++)
    // {
    //     *(_currentScreenFB + i) &= *(_pendingScreenFB + i);
    //     *(_currentScreenFB + i) |= *(_pendingScreenFB + i);
    // }

    // // Disable EPD PSU if needed.
    // if (!_leaveOn)
    //     epdPSU(0);
}

void EPDDriver::display(uint8_t _leaveOn)
{
    // Depending on the mode, use on or the other function.
    if (getDisplayMode() == INKPLATE_1BW)
    {
        INKPLATE_DEBUG_MGS("1bit global update");
        display1b(_leaveOn);
        INKPLATE_DEBUG_MGS("1bit global update done");
    }
    else
    {
        INKPLATE_DEBUG_MGS("4bit global update");
        display4b(_leaveOn);
        INKPLATE_DEBUG_MGS("4bit global update done");
    }
}

// Display content from RAM to display (1 bit per pixel,. monochrome picture).
void EPDDriver::display1b(uint8_t _leaveOn)
{
    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Full update? Copy everything in screen buffer before refresh!
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(&hmdma_mdma_channel40_sw_0, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 8));

    // Pointer to the framebuffer (used by the fast GLUT). It gets 8 pixels from the framebuffer.
    uint8_t *_fbPtr;

    // Do a clear sequence!
    cleanFast(_waveform1BitInternal.clearLUT, _waveform1BitInternal.clearPhases);

    for (int k = 0; k < _waveform1BitInternal.lutPhases; k++)
    {
        // Set the pointer at the start of the framebuffer.
        volatile uint8_t *ptr = _pendingScreenFB;

        // Set the current lut for the wavefrom.
        uint8_t *_currentWfLut = ((uint8_t**)default1BitWavefrom.lut)[k];

        // Get the 64 rows of the data (faster RAM read speed, since it reads whole RAM column at once).
        // Reading line by line will gets us only 89MB/s read speed, but reading 64 rows or more at once will get us
        // ~215MB/s read speed! Nice! Start the DMA transfer!
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_pendingScreenFB, (uint32_t)_oneLine1,
                          sizeof(_oneLine1), 1);
        while (stm32FMCSRAMCompleteFlag() == 0)
            ;
        stm32FMCClearSRAMCompleteFlag();
        ptr += sizeof(_oneLine1);

        // Set the current working RAM buffer to the first RAM Buffer (_oneLine1).
        _fbPtr = (uint8_t *)_oneLine1;

        // Decode the first line.
        for (int n = 0; n < (SCREEN_WIDTH / 4); n += 2)
        {
            _decodedLine1[n] = _currentWfLut[(*_fbPtr) >> 4];
            _decodedLine1[n + 1] = _currentWfLut[(*(_fbPtr++)) & 0x0F];
        }

        // Set the pointers for double buffering.
        _pendingDecodedLineBuffer = _decodedLine2;
        _currentDecodedLineBuffer = _decodedLine1;

        // Send to the screen!
        vScanStart();
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            hScanStart(_currentDecodedLineBuffer[0], _currentDecodedLineBuffer[1]);
            HAL_MDMA_Start_IT(&hmdma_mdma_channel41_sw_0, (uint32_t)_currentDecodedLineBuffer + 2,
                              (uint32_t)EPD_FMC_ADDR, sizeof(_decodedLine1), 1);

            // Decode the pixels into Waveform for EPD.
            for (int n = 0; n < (SCREEN_WIDTH / 4); n += 2)
            {
                _pendingDecodedLineBuffer[n] = _currentWfLut[(*_fbPtr) >> 4];
                _pendingDecodedLineBuffer[n + 1] = _currentWfLut[(*(_fbPtr++)) & 0x0F];
            }

            // Swap the buffers!
            if (_currentDecodedLineBuffer == _decodedLine1)
            {
                _currentDecodedLineBuffer = _decodedLine2;
                _pendingDecodedLineBuffer = _decodedLine1;
            }
            else
            {
                _currentDecodedLineBuffer = _decodedLine1;
                _pendingDecodedLineBuffer = _decodedLine2;
            }

            // Can't start new transfer until all data is sent to EPD.
            while (stm32FMCEPDCompleteFlag() == 0)
                ;
            stm32FMCClearEPDCompleteFlag();

            // Advance the line on EPD.
            vScanEnd();

            // Check if the buffer needs to be updated (after 64 lines).
            if ((i & 0b00111111) == 0b00111110)
            {
                // Update the buffer pointer.
                _fbPtr = (uint8_t *)_oneLine1;

                // Start new RAM DMA transfer.
                HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)ptr, (uint32_t)(_oneLine1), sizeof(_oneLine1),
                                  1);

                ptr += sizeof(_oneLine1);

                // Wait for DMA transfer to complete.
                while (stm32FMCSRAMCompleteFlag() == 0)
                    ;
                stm32FMCClearSRAMCompleteFlag();
            }
        }
    }

    // Full update done? Allow for partial updates.
    _blockPartial = 0;

    // End refresh sequence.
    //cleanFast(3, 1);

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);
}

void EPDDriver::display4b(uint8_t _leaveOn)
{
    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Full update? Copy everything in screen buffer before refresh!
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(&hmdma_mdma_channel40_sw_0, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 2));

    // Pointer to the framebuffer (used by the fast GLUT). It gets 4 pixels from the framebuffer.
    uint16_t *_fbPtr;

    // Do a clear sequence!
    cleanFast(_waveform4BitInternal.clearLUT, _waveform4BitInternal.clearPhases);

    for (int k = 0; k < _waveform4BitInternal.lutPhases; k++)
    {
        volatile uint8_t *ptr = _pendingScreenFB;

        // First calculate the new fast GLUT for the current EPD waveform phase.
        calculateGLUTOnTheFly(_fastGLUT, ((uint8_t*)(default4BitWavefrom.lut + ((unsigned long)(k) << 4))));

        // Get the 16 rows of the data (faster RAM read speed, since it reads whole RAM column at once).
        // Reading line by line will gets us only 89MB/s read speed, but reading 16 rows or more at once will get us
        // ~215MB/s read speed! Nice! Start the DMA transfer!
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_pendingScreenFB, (uint32_t)_oneLine1,
                          sizeof(_oneLine1), 1);
        while (stm32FMCSRAMCompleteFlag() == 0)
            ;
        stm32FMCClearSRAMCompleteFlag();
        ptr += sizeof(_oneLine1);

        // Set the current working RAM buffer to the first RAM Buffer (_oneLine1).
        _fbPtr = (uint16_t *)_oneLine1;

        // Decode the first line.
        for (int n = 0; n < (SCREEN_WIDTH / 4); n++)
        {
            _decodedLine1[n] = _fastGLUT[*(_fbPtr++)];
        }

        // Set the pointers for double buffering.
        _pendingDecodedLineBuffer = _decodedLine2;
        _currentDecodedLineBuffer = _decodedLine1;

        // Send to the screen!
        vScanStart();
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            hScanStart(_currentDecodedLineBuffer[0], _currentDecodedLineBuffer[1]);
            HAL_MDMA_Start_IT(&hmdma_mdma_channel41_sw_0, (uint32_t)_currentDecodedLineBuffer + 2,
                              (uint32_t)EPD_FMC_ADDR, sizeof(_decodedLine1), 1);

            // Decode the pixels into Waveform for EPD.
            for (int n = 0; n < (SCREEN_WIDTH / 4); n++)
            {
                _pendingDecodedLineBuffer[n] = _fastGLUT[*(_fbPtr++)];
            }

            // Swap the buffers!
            if (_currentDecodedLineBuffer == _decodedLine1)
            {
                _currentDecodedLineBuffer = _decodedLine2;
                _pendingDecodedLineBuffer = _decodedLine1;
            }
            else
            {
                _currentDecodedLineBuffer = _decodedLine1;
                _pendingDecodedLineBuffer = _decodedLine2;
            }

            // Can't start new transfer until all data is sent to EPD.
            while (stm32FMCEPDCompleteFlag() == 0)
                ;
            stm32FMCClearEPDCompleteFlag();

            // Advance the line on EPD.
            vScanEnd();

            // Check if the buffer needs to be updated (after 16 lines).
            if ((i & 0b00001111) == 0b00001110)
            {
                // Update the buffer pointer.
                _fbPtr = (uint16_t *)_oneLine1;

                // Start new RAM DMA transfer.
                HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)ptr, (uint32_t)(_oneLine1), sizeof(_oneLine1),
                                  1);

                ptr += sizeof(_oneLine1);

                // Wait for DMA transfer to complete.
                while (stm32FMCSRAMCompleteFlag() == 0)
                    ;
                stm32FMCClearSRAMCompleteFlag();
            }
        }
    }

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);
}

bool EPDDriver::loadWaveform(InkplateWaveform _customWaveform)
{
    // Do a few checks.
    if ((_customWaveform.lut == NULL) || (_customWaveform.lutPhases == 0) || (_customWaveform.tag != 0xef)) return false;

    // Check if the waveform is used on 1 bit mode or 4 bit mode.
    if (_customWaveform.mode == INKPLATE_WF_1BIT)
    {
        _waveform1BitInternal = _customWaveform;
    }
    else
    {
        _waveform4BitInternal = _customWaveform;
    }

    // Return 1 for success.
    return true;
}

int EPDDriver::epdPSU(uint8_t _state)
{
    // Check if the atate is already set.
    if (_state == _epdPSUState)
        return 1;

    // Enable the EPD power supply
    if (_state)
    {
        // Set EPD PMIC to high.
        internalIO.digitalWriteIO(TPS_WAKE_PIN, HIGH, true);
        delay(5);
        internalIO.digitalWriteIO(TPS_PWRUP_PIN, HIGH, true);
        delay(5);

        // Enable all rails.
        pmic.setRails(0b00111111);

        // Configure GPIO pins for defaullt state after power up.
        epdGpioState(EPD_DRIVER_PINS_OUTPUT);
        LE_CLEAR;
        SPH_SET;
        GMODE_SET;
        SPV_SET;
        CKV_CLEAR;
        OE_SET;
        internalIO.digitalWriteIO(TPS_VCOM_CTRL_PIN, HIGH, true);

        // Wait until EPD PMIC has all needed voltages at it's outputs.
        // 250 ms should be long enough.
        unsigned long timer = millis();
        do
        {
            delay(1);
        } while ((pmic.getPwrgoodFlag() != TPS651851_PWR_GOOD_OK) && (millis() - timer) < 1000ULL);

        // Not ready even after 250ms? Something is wrong, shut down TPS!
        if (pmic.getPwrgoodFlag() != TPS651851_PWR_GOOD_OK)
        {
            internalIO.digitalWriteIO(TPS_VCOM_CTRL_PIN, LOW, true);
            internalIO.digitalWriteIO(TPS_PWRUP_PIN, LOW, true);
            INKPLATE_DEBUG_MGS("EPC PMIC power up failed");
            return 0;
        }

        // Enable buffer for the control ePaper lines.
        EPD_BUF_CLEAR;

        // Set new PMIC state.
        _epdPSUState = 1;
    }
    else
    {
        // Shut down req.

        // Set all GPIO pins to lov (to avoid epaper display damage).
        OE_CLEAR;
        GMODE_CLEAR;
        CKV_CLEAR;
        SPH_CLEAR;
        SPV_CLEAR;
        LE_CLEAR;
        internalIO.digitalWriteIO(TPS_VCOM_CTRL_PIN, LOW, true);
        internalIO.digitalWriteIO(TPS_PWRUP_PIN, LOW, true);

        // Disable buffer for the control ePaper lines.
        EPD_BUF_SET;

        // 250ms should be long enough to shut down all EPD PMICs voltage rails.
        unsigned long timer = millis();
        do
        {
            delay(1);
        } while ((pmic.getPwrgoodFlag() != 0) && (millis() - timer) < 1000ULL);

        // Disable all rails.
        pmic.setRails(0b00000000);

        // There is still voltages at the EPD PMIC? Something does not seems right...
        if (pmic.getPwrgoodFlag() != 0)
        {
            INKPLATE_DEBUG_MGS("EPC PMIC power down failed");
            return 0;
        }

        epdGpioState(EPD_DRIVER_PINS_H_ZI);

        internalIO.digitalWriteIO(TPS_VCOM_CTRL_PIN, LOW, true);

        // Set new PMIC state.
        _epdPSUState = 0;
    }

    // Everything went ok? Return 1 as success.
    return 1;
}

void EPDDriver::gpioInit()
{
    // Disable user usage on some GPIO expander pins.
    // APDS interrupt pin.
    internalIO.blockPinUsage(0);
    // TPS EPD PMIC Wakeup.
    internalIO.blockPinUsage(3);
    // TPS EPD PMIC PWRUP.
    internalIO.blockPinUsage(4);
    // TPS EPD PMIC VCOM_CTRL.
    internalIO.blockPinUsage(5);

    // Set EPD buffer enable for ePaper control pins to output.
    pinMode(EPD_BUFF_PIN, OUTPUT);

    // Enable the external RAM (inverse logic due P-MOS) and enable it by default.
    pinMode(RAM_EN_GPIO, OUTPUT);
    digitalWrite(RAM_EN_GPIO, LOW);

    // Disable battery measurement pin
    pinMode(BATTERY_MEASUREMENT_EN, OUTPUT);
    digitalWrite(BATTERY_MEASUREMENT_EN, LOW);

    // Set battery measurement pin as analog input.
    pinMode(ANALOG_BATTERY_MEASUREMENT, INPUT_ANALOG);

    // Set TPS control pins to outputs.
    internalIO.pinModeIO(TPS_PWRUP_PIN, OUTPUT, true);
    internalIO.pinModeIO(TPS_WAKE_PIN, OUTPUT, true);
    internalIO.pinModeIO(TPS_VCOM_CTRL_PIN, OUTPUT, true);

    // Set pin for the AS5600 power MOSFET and disable it.
    internalIO.pinModeIO(PERIPHERAL_POSITIONENC_ENABLE_PIN, OUTPUT, true);
    internalIO.digitalWriteIO(PERIPHERAL_POSITIONENC_ENABLE_PIN, LOW, true);

    // Set the type of the EPD control pins.
    pinMode(EPD_CKV_GPIO, OUTPUT);
    pinMode(EPD_SPV_GPIO, OUTPUT);
    pinMode(EPD_SPH_GPIO, OUTPUT);
    pinMode(EPD_OE_GPIO, OUTPUT);
    pinMode(EPD_GMODE_GPIO, OUTPUT);
    pinMode(EPD_LE_GPIO, OUTPUT);
}

double EPDDriver::readBattery()
{
    // Enable MOSFET for viltage divider.
    digitalWrite(BATTERY_MEASUREMENT_EN, HIGH);

    // Wait a little bit.
    delay(1);

    // Get an voltage measurement from ADC.
    uint16_t _adcRaw = analogRead(ANALOG_BATTERY_MEASUREMENT);

    // Disable MOSFET for voltage divider (to save power).
    digitalWrite(BATTERY_MEASUREMENT_EN, LOW);

    // Calculate the voltage from ADC measurement. Divide by 2^16) - 1 to get
    // measurement voltage in the form of the percentage of the ADC voltage,
    // multiply it by analog reference voltage and multiply by two (voltage divider).
    double _voltage = (double)(_adcRaw) / 65535.0 * 3.3 * 2;

    // Return the measured voltage.
    return _voltage;
}

void EPDDriver::epdGpioState(uint8_t _state)
{
    if (_state)
    {
        // Set all pins to input (Hi-Z state).
        pinMode(EPD_GMODE_GPIO, INPUT);
        pinMode(EPD_CKV_GPIO, INPUT);
        pinMode(EPD_SPV_GPIO, INPUT);
        pinMode(EPD_SPH_GPIO, INPUT);
        pinMode(EPD_OE_GPIO, INPUT);
        pinMode(EPD_LE_GPIO, INPUT);
    }
    else
    {
        // Set all pins to the output.
        pinMode(EPD_GMODE_GPIO, OUTPUT);
        pinMode(EPD_CKV_GPIO, OUTPUT);
        pinMode(EPD_SPV_GPIO, OUTPUT);
        pinMode(EPD_SPH_GPIO, OUTPUT);
        pinMode(EPD_OE_GPIO, OUTPUT);
        pinMode(EPD_LE_GPIO, OUTPUT);
    }
}

uint32_t EPDDriver::differenceMask(uint8_t *_currentScreenFB, uint8_t *_pendingScreenFB, uint8_t *_differenceMask)
{
    // Try to find the difference between two frame buffers.
    // Idea is this: find the difference between two framebuffers, simple!
    // Do this by by first calculating difference on framebuffers itself. Use bitwise XOR operation (_currentScreenFB
    // XOR _pendingScreenFB). Convert this mask into EPD mask array. 1 convert to 00 and 0 to 11. Yes, output data will
    // be two times larger since fraamebuffer is packed as 1BPP and EPD as 2BPP. Now, use pending framebuffer and do OR
    // operation with EPD mask array. In this case any same pixels on both framebuffers will have '11' bit combinaiton
    // (11 on epaper means skip this pixel, do not change it), any other will get the state of the pending framebuffer.
    // Example:
    // _currentScreenFB = 0b11001010
    // _pendingScreenFB = 0b00101101
    // _differenceMask  = 0b11100111
    // _differenceMask -> _differenceEDPMask = 0b0000001111000000
    // _pendingScreenFB -> _pendingScreenEPD = 0b1010011001011001
    // _finalEPDData = _pendingScreenEPD | _differenceEDPMask = 0b1010011111011001 -> WWBSSBWB (W = New White Pixel, B =
    // New Black Pixel S = Skip Pixel).

    // Set the offset for the framebuffer address.
    uint32_t _fbAddressOffset = 0;

    // Used for counting how many pixels will change.
    uint32_t _change = 0;

    // Using a pointer, interpret 8 bit array as 16 bit array.
    uint16_t *_outDataArray = (uint16_t*)_oneLine3;

    while (_fbAddressOffset < ((SCREEN_HEIGHT * SCREEN_WIDTH / 8)))
    {
        // Get the 64 lines from the current screen buffer into internal RAM.
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_currentScreenFB + _fbAddressOffset, (uint32_t)_oneLine1, sizeof(_oneLine1), 1);
        while (stm32FMCSRAMCompleteFlag() == 0);
        stm32FMCClearSRAMCompleteFlag();

        // Copy 64 lines from pending framebuffer of the EPD.
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_pendingScreenFB + _fbAddressOffset, (uint32_t)_oneLine2, sizeof(_oneLine2), 1);
        while (stm32FMCSRAMCompleteFlag() == 0);
        stm32FMCClearSRAMCompleteFlag();

        // Find the difference between two framebuffers and make EPD mask!
        for (uint32_t i = 0; i < sizeof(_oneLine1); i++)
        {
            uint8_t _pixelMask = _oneLine1[i] ^ _oneLine2[i];
            uint16_t epdPixelData = LUTBW[_oneLine2[i] >> 4] << 8 | LUTBW[_oneLine2[i] & 0x0F];
            uint16_t outData = LUTP[_pixelMask >> 4] << 8 | LUTP[_pixelMask & 0x0F];
            uint16_t maskedOutData = outData | epdPixelData;
            _outDataArray[i] = (maskedOutData >> 8) | (maskedOutData << 8);
        }

        // Send data to the difference mask. Difference mask for EPD is two times larger than the framebuffer for 1 bit
        // mode.
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_oneLine3, (uint32_t)(_differenceMask) + (_fbAddressOffset << 1), sizeof(_oneLine3), 1);
        while (stm32FMCSRAMCompleteFlag() == 0);
        stm32FMCClearSRAMCompleteFlag();

        // Update the pointer.
        _fbAddressOffset += sizeof(_oneLine1);
    }

    // Return number of pixels needed to change.
    return _change;
}

void EPDDriver::drawBitmapFast(const uint8_t *_p)
{
    // For now, image must be in full resolution of the screen!
    // To-Do: Add x, y, w and h.
    // To-Do2: Check for input parameters.
    // To-Do3: Check for screen rotation!

    // Copy line by line.
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH / 8; i += sizeof(_oneLine1))
    {
        // Copy data into internal buffer.
        memcpy(_oneLine1, _p + i, sizeof(_oneLine1));

        // Start DMA transfer into pending screen framebuffer.
        HAL_MDMA_Start_IT(&hmdma_mdma_channel40_sw_0, (uint32_t)_oneLine1, (uint32_t)(_pendingScreenFB) + i, sizeof(_oneLine1), 1);
        while (stm32FMCSRAMCompleteFlag() == 0);
        stm32FMCClearSRAMCompleteFlag();
    }
}

void EPDDriver::calculateGLUTOnTheFly(uint8_t *_lut, uint8_t *_waveform)
{
    for (uint32_t i = 0; i < 65536; i++)
    {
        uint32_t index0 = (i & 0x0F);
        uint32_t index1 = ((i >> 4) & 0x0F);
        uint32_t index2 = ((i >> 8) & 0x000F);
        uint32_t index3 = ((i >> 12) & 0x000F);

        _lut[i] = (_waveform[index3]) | (_waveform[index2] << 2);
        _lut[i] |= (_waveform[index1] << 4) | (_waveform[index0] << 6);
    }
}

void EPDDriver::selectDisplayMode(uint8_t _mode)
{
    // Block the parameter to only two possible values.
    _mode &= 1;

    // Check if the mode has changed. If not, ignore it.
    if (_mode == _displayMode) return;

    // Copy the new value.
    _displayMode = _mode;

    // If the change in mode is detected, flush/clear the frame buffers.
    clearDisplay();

    // Force clearing current screen buffer. 
    copySDRAMBuffers(&hmdma_mdma_channel40_sw_0, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 8));
    
    // Block the partial updates.
    _blockPartial = 1;
}

uint8_t EPDDriver::getDisplayMode()
{
    return _displayMode;
}

void EPDDriver::peripheral(uint8_t _selectedPeripheral, bool _en)
{
    // Check what peripheral needs to be disabled or enabled.
    switch (_selectedPeripheral)
    {
        case INKPLATE_ROTARY_ENCODER_PERIPH:
            internalIO.digitalWriteIO(PERIPHERAL_POSITIONENC_ENABLE_PIN, _en, true);
            Serial.println("Enc on!");
            break;
        case INKPLATE_WS_LED_PERIPH:
            internalIO.digitalWriteIO(PERIPHERAL_WSLED_ENABLE_PIN, _en, true);
            break;
    }
}

void EPDDriver::setFullUpdateTreshold(uint16_t _numberOfPartialUpdates)
{
    // Copy the value into the local variable.
    _partialUpdateLimiter = _numberOfPartialUpdates;

    // If the limiter is enabled, force full update.
    if (_numberOfPartialUpdates != 0) _blockPartial = true;
}

#endif