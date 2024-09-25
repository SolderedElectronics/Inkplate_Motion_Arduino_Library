/**
 **************************************************
 *
 * @file        IP6MotionDriver.h
 * @brief       
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include main header ffile for the driver.
#include "IP6MotionDriver.h"

// Include the WiFi Library.
#include "../../system/wifi/esp32SpiAt.h"

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

// STM32 SPI class for internal Inkplate SPI (used by microSD card and WiFi).
// Must be declared this way, for some reason, it hangs when used new operator on microSD card init.
SPIClass _inkplateSystemSPI(INKPLATE_MICROSD_SPI_MOSI, INKPLATE_MICROSD_SPI_MISO, INKPLATE_MICROSD_SPI_SCK);

/**
 * @brief Constructor for EPDDriver object.
 * 
 */
EPDDriver::EPDDriver()
{
    // Empty constructor.
}

/**
 * @brief   ePaper driver initializer for the Inkplate 6 Motion board.
 * 
 * @return  int
 *          0 = Initialization has faild, check debug messages for more info.
 *          1 = Initialization ok. 
 */
int EPDDriver::initDriver()
{
    INKPLATE_DEBUG_MGS("EPD Driver init started");

    // Get the instances for DMA.
    _epdMdmaHandle  = stm32FmcGetEpdMdmaInstance();
    _sdramMdmaHandle  = stm32FmcGetSdramMdmaInstance();

    // Configure IO expander.
    if (!internalIO.beginIO(IO_EXPANDER_INTERNAL_I2C_ADDR))
    {
        INKPLATE_DEBUG_MGS("GPIO expander init fail");
    }

    // Initialize every Inkplate 6 Motion peripheral.
    shtc3.begin();
    lsm6ds3.begin();
    apds9960.init();

    // Put every peripheral into low power mode.
    peripheralState(INKPLATE_PERIPHERAL_ALL_PERI, false);

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

    // Init STM32 FMC (Flexible memory controller) for faster pushing data to panel using hardware.
    stm32FmcInit(EPD_FMC_ADDR);

    // Turn off EPD PMIC.
    internalIO.digitalWriteIO(TPS_WAKE_PIN, LOW, true);

    // Setup SPI config for the SdFat library for the STM32.
    _microSDCardSPIConf = new SdSpiConfig(INKPLATE_MICROSD_SPI_CS, SHARED_SPI, SD_SCK_MHZ(20), &_inkplateSystemSPI);

    INKPLATE_DEBUG_MGS("EPD Driver init done");

    // Everything went ok? Return 1 for success.
    return 1;
}

/**
 * @brief   Method for clearing the contennt form the screen.
 * 
 * @param   uint8_t *_clearWavefrom
 *          Waveform look up table for clearing the screen.
 * @param   _wavefromPhases _wavefromPhases
 *          how many phases are needed to clean the screen (it's related to the waveform!).
 * @note    For more info about the waveforms, see waveforms.h! Also, this function keeps EPD PMIC
 *          on, it's up to the user to turn off the PMIC!
 */
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
            HAL_MDMA_Start_IT(_epdMdmaHandle, (uint32_t)_decodedLine1, (uint32_t)EPD_FMC_ADDR,
                              sizeof(_decodedLine1) - 2, 1);

            // Wait until the transfer has ended.
            while (!stm32FmcEpdCompleteFlag())
                ;

            // Clear the flag.
            stm32FmcClearEpdCompleteFlag();

            // End the line write.
            vScanEnd();
        }
    }

    // EPD PSU won't be turned off here after update.
    // It needs to be additionally or manually turned off.
}

/**
 * @brief   Clears content from the internal frame buffer, but not on the screen itself.
 * 
 */
void EPDDriver::clearDisplay()
{
    // Framebuffer if filled with different data depending on the cuurrent mode.
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

/**
 * @brief   Partailly update the screen. Remove and add only necessary changes.
 *          Also, do not clear the whole screen (screen won't flash in 1 bit mode).
 * 
 * @param   uint8_t _leaveOn
 *          0 = Shut down EPD power supply to save the power (but slower refresh due PMIC start-up time).
 *          1 = Keep EPD PMIC active after ePaper refresh.
 */
void EPDDriver::partialUpdate(uint8_t _leaveOn)
{
    // Automatically select partial update method depending on the screen mode (1 bit or 4 bit),
    if (getDisplayMode() == INKPLATE_1BW)
    {
        partialUpdate1Bit(_leaveOn);
    }
    else
    {
        partialUpdate4Bit(_leaveOn);
    }
}

/**
 * @brief   Partailly update the screen. Remove and add only necessary changes.
 *          Use a rapid clean to speed up the clean process of the pixels that will be changed.
 * 
 * @param   uint8_t _leaveOn
 *          0 = Shut down EPD power supply to save the power (but slower refresh due PMIC start-up time).
 *          1 = Keep EPD PMIC active after ePaper refresh.
 */
void EPDDriver::partialUpdate4Bit(uint8_t _leaveOn)
{
    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Check the mode.
    if (getDisplayMode() != INKPLATE_GL16)
        return;

    // Main princaple of the 4 bit partial update is to first clear all pixels
    // by setting them all into white color using custom waveform.

    // Pointer to the framebuffer (used by the fast GLUT). It gets 4 pixels from the framebuffer.
    uint16_t *_fbPtr;

    // Workaround to avod copying the same code with some minor changes.
    uint8_t *_wf[] = {(uint8_t*)(_waveform4BitPartialInternal.clearLUT), (uint8_t*)(_waveform4BitPartialInternal.lut)};
    uint16_t _phases[] = {_waveform4BitPartialInternal.clearPhases, _waveform4BitPartialInternal.lutPhases};
    __IO uint8_t *_fb[] = {_currentScreenFB, _pendingScreenFB};
    uint32_t _lineLoadTimings[] = {_waveform4BitPartialInternal.clearCycleDelay, _waveform4BitPartialInternal.cycleDelay};

    // First operations is cleaning old pixels from the ePaper, second is writing new pixels to the ePaper.
    for (int _operation = 0; _operation < 2; _operation++)
    {
        // Go trough the phases of the epaper update wavefrom.
        for (int k = 0; k < _phases[_operation]; k++)
        {
            // Load the line load timings - the slower, the better image quality.
            _lineWriteWaitCycles = _lineLoadTimings[_operation];

            // First calculate the new fast GLUT for the current EPD waveform phase.
            calculateGLUTOnTheFly(_fastGLUT, ((uint8_t*)(_wf[_operation]) + ((unsigned long)(k) << 4)));

            // Decode and send the pixels to the ePaper.
            pixelsUpdate(_fb[_operation], _fastGLUT, pixelDecode4BitEPD, 15, 2);
        }
    }

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);

    // Update the current framebuffer! Use DMA to transfer framebuffers.
    copySDRAMBuffers(_sdramMdmaHandle, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 2));
}

void EPDDriver::partialUpdate1Bit(uint8_t _leaveOn)
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
    differenceMask((uint8_t *)_currentScreenFB, (uint8_t *)_pendingScreenFB, (uint8_t *)_scratchpadMemory);

    // Load the timing.
    _lineWriteWaitCycles = _waveform1BitPartialInternal.cycleDelay;

    // Do the epaper phases.
    for (int k = 0; k < _waveform1BitPartialInternal.lutPhases; k++)
    {
        pixelsUpdate(_scratchpadMemory, NULL, pixelDecode1BitEPDPartial, 31, 4);
    }

    // Discharge the e-paper display.
    uint8_t _discharge = 0;
    cleanFast(&_discharge, 1);

    // Copy everything in current screen framebuffer.
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(_sdramMdmaHandle, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB, (SCREEN_WIDTH * SCREEN_HEIGHT / 8));

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

/**
 * @brief   Update the whole screen using global updates. This means the whole screen
 *          will flicker to clean the previous image.
 * 
 * @param   uint8_t _leaveOn
 *          0 = Shut down EPD power supply to save the power (but slower refresh due PMIC start-up time).
 *          1 = Keep EPD PMIC active after ePaper refresh. 
 */
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


/**
 * @brief   Use global 1 bit full update to update the content on the screen.
 *          Waveform for the 1 bit mode can be changed.
 * 
 * @param   uint8_t _leaveOn
 *          0 = Shut down EPD power supply to save the power (but slower refresh due PMIC start-up time).
 *          1 = Keep EPD PMIC active after ePaper refresh. 
 */
void EPDDriver::display1b(uint8_t _leaveOn)
{
    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Full update? Copy everything in screen buffer before refresh!
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(_sdramMdmaHandle, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 8));

    // Pointer to the framebuffer (used by the fast GLUT). It gets 8 pixels from the framebuffer.
    uint8_t *_fbPtr;
    
    // Use line write timing for the clear.
    _lineWriteWaitCycles = _waveform1BitInternal.clearCycleDelay;

    // Do a clear sequence!
    cleanFast(_waveform1BitInternal.clearLUT, _waveform1BitInternal.clearPhases);

    // Now use timing for the 1 bit full update.
    _lineWriteWaitCycles = _waveform1BitInternal.cycleDelay;

    for (int k = 0; k < _waveform1BitInternal.lutPhases; k++)
    {
        // Set the current lut for the wavefrom.
        uint8_t *_currentWfLut = ((uint8_t**)default1BitWavefrom.lut)[k];

        pixelsUpdate(_pendingScreenFB, _currentWfLut, pixelDecode1BitEPDFull, 63, 8);
    }

    // Full update done? Allow for partial updates.
    _blockPartial = 0;

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);
}

/**
 * @brief   Use global 4 bit full update to update the content on the screen.
 *          Waveform for the 4 bit mode can be changed. Image will be displayed
 *          in grayscale.
 * 
 * @param   uint8_t _leaveOn
 *          0 = Shut down EPD power supply to save the power (but slower refresh due PMIC start-up time).
 *          1 = Keep EPD PMIC active after ePaper refresh. 
 */
void EPDDriver::display4b(uint8_t _leaveOn)
{
    // Power up EPD PMIC. Abort update if failed.
    if (!epdPSU(1))
        return;

    // Full update? Copy everything in screen buffer before refresh!
    // Use DMA to transfer framebuffers!
    copySDRAMBuffers(_sdramMdmaHandle, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 2));

    // Use line write timing for the clear.
    _lineWriteWaitCycles = _waveform4BitInternal.clearCycleDelay;

    // Do a clear sequence!
    cleanFast(_waveform4BitInternal.clearLUT, _waveform4BitInternal.clearPhases);

    // Set waveform.
    default4BitWavefrom.lut = (uint8_t*)&(waveform4BitPartialLUT[0]);

    // Now use timing for the 4 bit full update.
    _lineWriteWaitCycles = _waveform4BitInternal.cycleDelay;

    for (int k = 0; k < _waveform4BitInternal.lutPhases; k++)
    {
        // First calculate the new fast GLUT for the current EPD waveform phase.
        calculateGLUTOnTheFly(_fastGLUT, ((uint8_t*)(_waveform4BitInternal.lut) + ((unsigned long)(k) << 4)));
        pixelsUpdate(_pendingScreenFB, _fastGLUT, pixelDecode4BitEPD, 15, 2);
    }

    // Disable EPD PSU if needed.
    if (!_leaveOn)
        epdPSU(0);
}

/**
 * @brief   Loads custom wavefrom in Inkplate 6 Motion Driver
 * 
 * @param   InkplateWaveform _customWaveform
 *          Custom wavefrom / non-default one. Check wavefroms.h file for more info!
 * @return  bool
 *          true - Waveform loaded successfully
 *          false - Wavefrom load failed
 * @note    Improper usage of this feature can PERMANETLY DAMAGE THE DISPLAY.
 *          Use it of your own risk! Preloaded waveforms from wavefroms.h are safe.
 */
bool EPDDriver::loadWaveform(InkplateWaveform _customWaveform)
{
    // Do a few checks.
    if ((_customWaveform.lutPhases == 0) || (_customWaveform.tag != 0xef)) return false;

    // Check if the waveform is used on 1 bit mode or 4 bit mode.
    if (_customWaveform.mode == INKPLATE_WF_1BIT)
    {
        // Check if the 1 bit partial update waveform is used or for global update.
        if (_customWaveform.type == INKPLATE_WF_PARTIAL_UPDATE)
        {
            // Copy it internally.
            memcpy(&_waveform1BitPartialInternal, &_customWaveform, sizeof(InkplateWaveform));
        }
        else
        {
            // Check for the LUTs. If is null, return error.
            if (_customWaveform.lut == NULL || _customWaveform.clearLUT == NULL) return 0;

            // Copy it internally.
            memcpy(&_waveform1BitInternal, &_customWaveform, sizeof(InkplateWaveform));
        }
    }
    else
    {
        // Check if the 1 bit partial update waveform is used or for global update.
        if (_customWaveform.type == INKPLATE_WF_PARTIAL_UPDATE)
        {
            // Copy it internally.
            memcpy(&_waveform4BitPartialInternal, &_customWaveform, sizeof(InkplateWaveform));
        }
        else
        {
            // Check for the LUTs. If is null, return error.
            if (_customWaveform.lut == NULL || _customWaveform.clearLUT == NULL) return 0;

            // Copy it internally.
            memcpy(&_waveform4BitInternal, &_customWaveform, sizeof(InkplateWaveform));
        }
    }

    // Return 1 for success.
    return true;
}

/**
 * @brief   Enables or disables power rails and GPIOs to the ePaper bus.
 * 
 * @param   uint8_t _state
 *          1 = Enable the ePaper PSU and set GPIOs for ePaper data bus.
 *          0 = Disable the ePaper PSU and disable GPIOs for ePaper data bus.
 * @return  int
 *          1 = New state successfully set.
 *          0 = New state of the ePaper PSU and GPIOs has failed (usually due ePaper PSU).
 */
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
        // One second should be long enough.
        unsigned long timer = millis();
        do
        {
            delay(1);
        } while ((pmic.getPwrgoodFlag() != TPS651851_PWR_GOOD_OK) && (millis() - timer) < 1000ULL);

        // Not ready even after 1000ms? Something is wrong, shut down TPS!
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

        // One second should be long enough to shut down all EPD PMICs voltage rails.
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

        // Set all GPIO pins to the EPD to Hi-Z.
        epdGpioState(EPD_DRIVER_PINS_H_ZI);

        // Disable TPS..
        internalIO.digitalWriteIO(TPS_WAKE_PIN, LOW, true);

        // Set new PMIC state.
        _epdPSUState = 0;
    }

    // Everything went ok? Return 1 as success.
    return 1;
}

/**
 * @brief   Initializes all the GPIOs of the Inkplate 6 Motion (SDRAM Supply Enable,
 *          ePaper PSU GPIOs, WiFi GPIO and WiFi SPI, Rotary Encoder Power Enable etc).
 * 
 */
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
    pinMode(INKPLATE_SDRAM_EN, OUTPUT);
    digitalWrite(INKPLATE_SDRAM_EN, LOW);

    // Disable battery measurement pin
    pinMode(INKPLATE_BATT_MEASURE_EN, OUTPUT);
    digitalWrite(INKPLATE_BATT_MEASURE_EN, LOW);

    // Set battery measurement pin as analog input.
    pinMode(INKPLATE_BATT_MEASURE, INPUT_ANALOG);

    // Set TPS control pins to outputs.
    internalIO.pinModeIO(TPS_PWRUP_PIN, OUTPUT, true);
    internalIO.pinModeIO(TPS_WAKE_PIN, OUTPUT, true);
    internalIO.pinModeIO(TPS_VCOM_CTRL_PIN, OUTPUT, true);

    // Set pin for the AS5600 power MOSFET and disable it.
    internalIO.pinModeIO(INKPLATE_POSITION_ENC_EN, OUTPUT, true);
    internalIO.digitalWriteIO(INKPLATE_POSITION_ENC_EN, LOW, true);

    // Set the type of the EPD control pins.
    pinMode(EPD_CKV_GPIO, OUTPUT);
    pinMode(EPD_SPV_GPIO, OUTPUT);
    pinMode(EPD_SPH_GPIO, OUTPUT);
    pinMode(EPD_OE_GPIO, OUTPUT);
    pinMode(EPD_GMODE_GPIO, OUTPUT);
    pinMode(EPD_LE_GPIO, OUTPUT);

    // Set the SPI pin for the WiFi.
    WiFi.hwSetup(&_inkplateSystemSPI);
}

/**
 * @brief   Reads the battery voltage (main battery conneted to the JST connector).
 * 
 * @return  double
 *          Battery voltage in volts.
 * @note    This method assumes 16 bit resolution on the ADC. Library sets ADC to 16 bit resolution at the start.
 *          If the ADC resolution changes, battery voltage measurement will be incorrect.
 * 
 */
double EPDDriver::readBattery()
{
    // Enable MOSFET for viltage divider.
    digitalWrite(INKPLATE_BATT_MEASURE_EN, HIGH);

    // Wait a little bit.
    delay(1);

    // Get an voltage measurement from ADC.
    uint16_t _adcRaw = analogRead(INKPLATE_BATT_MEASURE);

    // Disable MOSFET for voltage divider (to save power).
    digitalWrite(INKPLATE_BATT_MEASURE_EN, LOW);

    // Calculate the voltage from ADC measurement. Divide by 2^16) - 1 to get
    // measurement voltage in the form of the percentage of the ADC voltage,
    // multiply it by analog reference voltage and multiply by two (voltage divider).
    double _voltage = (double)(_adcRaw) / 65535.0 * 3.3 * 2;

    // Return the measured voltage.
    return _voltage;
}

/**
 * @brief   Sets GPIOs of the ePaper control lines to output or Hi-Z.
 *          This is used by the epdPSU()
 * 
 * @param   uint8_t _state
 *          EPD_DRIVER_PINS_OUTPUT = ePaper pins are set as outputs.
 *          EPD_DRIVER_PINS_H_ZI = ePaper control pins are set to Hi-Z state to save
 *          the power.
 */
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

/**
 * @brief   Used by the 1 bit partial update, it calculates difference between what is currently
 *          on the screen and what is pending in the framebuffer only stores different pixels.
 *          Also, packs them in wavefrom ready to be sent to the ePaper using FMC.
 * 
 * @param   uint8_t *_currentScreenFB
 *          Pointer to the framebuffer that stores current content of the screen.
 * @param   uint8_t *_pendingScreenFB
 *          Pointer to the framebuffer with pending changes.
 * @param   uint8_t *_differenceMask
 *          Pointer to the framebuffer where to store difference between _currentScreenFB and _pendingScreenFB
 *          packed ready to be sent to the ePaper with STM32 FMC peripheral.
 */
void EPDDriver::differenceMask(uint8_t *_currentScreenFB, uint8_t *_pendingScreenFB, uint8_t *_differenceMask)
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

    // Using a pointer, interpret 8 bit array as 16 bit array.
    uint16_t *_outDataArray = (uint16_t*)_oneLine3;

    while (_fbAddressOffset < ((SCREEN_HEIGHT * SCREEN_WIDTH / 8)))
    {
        // Get the 64 lines from the current screen buffer into internal RAM.
        HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_currentScreenFB + _fbAddressOffset, (uint32_t)_oneLine1, sizeof(_oneLine1), 1);
        while (stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();

        // Copy 64 lines from pending framebuffer of the EPD.
        HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_pendingScreenFB + _fbAddressOffset, (uint32_t)_oneLine2, sizeof(_oneLine2), 1);
        while (stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();

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
        HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_oneLine3, (uint32_t)(_differenceMask) + (_fbAddressOffset << 1), sizeof(_oneLine3), 1);
        while (stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();

        // Update the pointer.
        _fbAddressOffset += sizeof(_oneLine1);
    }
}

/**
 * @brief   Used to draw a full screen image in frame buffer as fast as possible.
 *          Used by the 1 bit partial updates (the ultra fast ones).
 * 
 * @param   const uint8_t *_p
 *          Pointer to the image bitmap data.
 * 
 * @note    To-Do: Try to implement STM32 DMA2D for this.
 */
void EPDDriver::drawBitmapFast(const uint8_t *_p)
{
    // For now, image must be in full resolution of the screen!
    // To-Do: Add x, y, w and h.
    // To-Do2: Check for input parameters.
    // To-Do3: Check for screen rotation!
    // To-Do4: Use HW accelerator for all that if possible?

    // Copy line by line.
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH / 8; i += sizeof(_oneLine1))
    {
        // Copy data into internal buffer.
        memcpy(_oneLine1, _p + i, sizeof(_oneLine1));

        // Start DMA transfer into pending screen framebuffer.
        HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_oneLine1, (uint32_t)(_pendingScreenFB) + i, sizeof(_oneLine1), 1);
        while (stm32FmcSdramCompleteFlag() == 0);
        stm32FmcClearSdramCompleteFlag();
    }
}

/**
 * @brief   Initializes the microSD card on the Inkplate 6 Motion.
 * 
 * @return  bool
 *          true = Initialization is successfull.
 *          false = Initialization has failed.
 */
bool EPDDriver::microSDCardInit()
{
    // Power up the card!
    internalIO.pinModeIO(INKPLATE_MICROSD_PWR_EN, OUTPUT, true);

    // Set pin to low (PMOS is used as a switch).
    internalIO.digitalWriteIO(INKPLATE_MICROSD_PWR_EN, LOW, true);

    // Wait a little bit.
    delay(10);

    // Try to init microSD card!
    _microSdInit = sdFat.begin(*_microSDCardSPIConf);

    // Return the result of microSD card initializaton.
    return _microSdInit;
}

/**
 * @brief   Enable or dosable Inkplate 6 Motion peripherals to sve the power in sleep.
 * 
 * @param   uint8_t _peripheral
 *          Selected peripheral (INKPLATE_PERIPHERAL_SDRAM, INKPLATE_PERIPHERAL_ROTARY_ENCODER, etc).
 *          See all of them in featureSelect.h in (src/features).
 * @param   bool _en
 *          Set the state of the currently selected peripheral; true = enable it, false = disable it.
 */
void EPDDriver::peripheralState(uint8_t _peripheral, bool _en)
{
    // You can disable or enable multiple peripher. at once.
    // Check if SDRAM needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_SDRAM)
    {
        if (!_en)
        {
            // If SDRAM must be disabled, first de-init the SDRAM.
            stm32FmcDeInit();

            // Disable power to the SDRAM.
            digitalWrite(INKPLATE_SDRAM_EN, HIGH);
        }
        else
        {
            // First enable power to the SDRAM.
            digitalWrite(INKPLATE_SDRAM_EN, HIGH);

            // Re-init SDRAM.
            stm32FmcInit(EPD_FMC_ADDR);
        }
    }
    
    // Check if magnetic rotary encoder needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_ROTARY_ENCODER)
    {
        if (_en)
        {
            // If PWR MOSDET needs to be enabled, set controll pin from the IO Expander to output first.
            internalIO.pinModeIO(INKPLATE_POSITION_ENC_EN, OUTPUT, true);
            // Set the same pin to high, enabling power to the magnetic rotary encoder.
            internalIO.digitalWriteIO(INKPLATE_POSITION_ENC_EN, HIGH, true);
        }
        else
        {
            // If PWR MOSDET needs to be disabled, firsdt set the pin to LOW.
            internalIO.digitalWriteIO(INKPLATE_POSITION_ENC_EN, LOW, true);

            // Set it to input (external pull-down resistor on the gate will keep it low).
            internalIO.pinModeIO(INKPLATE_POSITION_ENC_EN, INPUT, true);
        }
    }

    // Check if both addressable RGB LED needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_WS_LED)
    {
        if (_en)
        {
            // Set pin connected to the PWR MOSFET gate to the output.
            internalIO.pinModeIO(INKPLATE_WSLED_EN, OUTPUT, true);

            // Set it to high, thus enabling the power to the LED.
            internalIO.digitalWriteIO(INKPLATE_WSLED_EN, HIGH, true);
        }
        else
        {
            // If needs to disabled, shut down power to the LED by pulling MOSFET gate to the GND.
            internalIO.digitalWriteIO(INKPLATE_WSLED_EN, LOW, true);

            // Set same GPIO to input, to be pulled externally by pull-down resistor.
            internalIO.pinModeIO(INKPLATE_WSLED_EN, INPUT, true);
        }
    }

    // Check if SHTC3 needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_SHTC3)
    {
        if (!_en)
        {
            // Send command for sleep.
            shtc3.sleep();
        }
        else
        {
            // Send commands for setting current mode: Polling, RH first, Normal power mode.
            shtc3.setMode(SHTC3_CMD_CSD_RHF_NPM);
        }
    }

    // Check if APDS9960 needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_APDS9960)
    {
       if (!_en)
        {
            // Send command for disable power to the APDS9960.
            apds9960.disablePower();
        }
        else
        {
            // Re-enable APDS9960.
            apds9960.enablePower();
        }
    }

    if (_peripheral & INKPLATE_PERIPHERAL_LSM6DS3)
    {
        if (!_en)
        {
            // Disable everything!
            lsm6ds3.settings.gyroEnabled = 0;
            lsm6ds3.settings.accelEnabled = 0;
            lsm6ds3.settings.tempEnabled = 0;

            // Update settings.
            lsm6ds3.begin(&lsm6ds3.settings);
        }
        else
        {
            // Enable all!
            lsm6ds3.settings.gyroEnabled = 1;
            lsm6ds3.settings.accelEnabled = 1;
            lsm6ds3.settings.tempEnabled = 1;

            // Update settings.
            lsm6ds3.begin(&lsm6ds3.settings);
        }
    }


    // Check if microSD needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_MICROSD)
    {
        if (_en)
        {
            // First, set control pin for the microSD power MOSFET to output.
            internalIO.pinModeIO(INKPLATE_MICROSD_PWR_EN, OUTPUT, true);

            // Send power to the microSD.
            internalIO.digitalWriteIO(INKPLATE_MICROSD_PWR_EN, LOW, true);
        }
        else
        {
            // First disable power to the microSD.
            internalIO.digitalWriteIO(INKPLATE_MICROSD_PWR_EN, HIGH, true);


            // Kepp the PWR MOSFET disabled with external pull-up resistor.
            internalIO.pinModeIO(INKPLATE_MICROSD_PWR_EN, INPUT, true);

            // Set variable for microSD card initializaton status to false (used for enabling/disabling WiFi).
            _microSdInit = false;
        }
    }

    // Check if ESP32 needs to be enabled/disabled.
    if (_peripheral & INKPLATE_PERIPHERAL_WIFI)
    {
        // This is only for the microSD card issue. MicroSD card and ESP32 share SPI bus.
        // If the card is inserted but it's not enabled, STM32 can't communicated with the
        // ESP32 since SPI bus is loaded with the microSD card. So, in the case of the usage of
        // the WiFi, microSD card needs to also be enabled.
        if (_en)
        {
            // First, set control pin for the microSD power MOSFET to output.
            internalIO.pinModeIO(INKPLATE_MICROSD_PWR_EN, OUTPUT, true);

            // Send power to the microSD.
            internalIO.digitalWriteIO(INKPLATE_MICROSD_PWR_EN, LOW, true);
        }
        else if (!_microSdInit && !_en)
        {
            // Only if the card is not currently in use, remove microSD card supply while disabling WiFi.
            // First disable power to the microSD.
            internalIO.digitalWriteIO(INKPLATE_MICROSD_PWR_EN, HIGH, true);

            // Kepp the PWR MOSFET disabled with external pull-up resistor.
            internalIO.pinModeIO(INKPLATE_MICROSD_PWR_EN, INPUT, true);
        }

    }
}

/**
 * @brief   Method calcuates fast look-up table for the 4 bit global update mode.
 *          It calculates the LUT for the current waveform phase.
 * 
 * @param   uint8_t *_lut
 *          Pointer to the array where to store calculated LUT (must be 65536 bytes).
 * @param   uint8_t *_waveform
 *          Waveform LUT, see wavefroms.h or pixelDecode4BitEPD() to see example usage.
 */
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

/**
 * @brief   Select current display mode.
 *          It can be 4 bit grayscale or 1 bit.
 * 
 * @param   uint8_t _mode
 *          INKPLATE_GL16 = 4 bit mode.
 *          INKPLATE_1BW = 1 bit mode.
 * 
 * @note    Once mode has been changed, all content of the framebuffers is cleared.
 */
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
    copySDRAMBuffers(_sdramMdmaHandle, _oneLine1, sizeof(_oneLine1), _pendingScreenFB, _currentScreenFB,
                     (SCREEN_WIDTH * SCREEN_HEIGHT / 8));
    
    // Block the partial updates.
    _blockPartial = 1;
}

/**
 * @brief   Get the current display mode (BW or grayscale).
 * 
 * @return  uint8_t
 *          INKPLATE_GL16 = 4 bit mode.
 *          INKPLATE_1BW = 1 bit mode.
 */
uint8_t EPDDriver::getDisplayMode()
{
    return _displayMode;
}

/**
 * @brief   Set the number of partial updates afterwhich full screen update is performed.
 * 
 * @param   uint16_t _numberOfPartialUpdates
 *          Number of allowed partial updates afterwhich full update is performed.
 *          0 = disabled, no automatic full update will be performed.
 * 
 * @note    By default, this is disabled, but to keep best image quality perform a full update
 *          every 60-80 partial updates.
 */
void EPDDriver::setFullUpdateTreshold(uint16_t _numberOfPartialUpdates)
{
    // Copy the value into the local variable.
    _partialUpdateLimiter = _numberOfPartialUpdates;

    // If the limiter is enabled, force full update.
    if (_numberOfPartialUpdates != 0) _blockPartial = true;
}

/**
 * @brief   Method that do it's magic to update the screen. It's universal for all modes.
 * 
 * @param   volatile uint8_t *_frameBuffer
 *          Pointer to the framebuffer storing pixels.
 * @param   uint8_t *_waveformLut
 *          Used LUT for converting from pixels into waveform.
 * @param   void (*_pixelDecode)(void*, void*, void*)
 *          Callback to the function that will be used for decoding pixels to wavefrom (data ready to be sent to the ePaper).
 * @param   const uint8_t _prebufferedLines
 *          How many lines to read at one from the framebuffer (SDRAM). It depends on BPP and buffer size (_oneLine1, _oneLine2, _oneLine3).
 * @param   uint8_t _pixelsPerByte
 *          How many pixels are stored in one byte inside framebuffer (4 bit = 2 pixels, 1 bit = 8 pixels).
 */
void EPDDriver::pixelsUpdate(volatile uint8_t *_frameBuffer, uint8_t *_waveformLut, void (*_pixelDecode)(void*, void*, void*), const uint8_t _prebufferedLines, uint8_t _pixelsPerByte)
{
        // Pointer to the framebuffer (used by the fast GLUT). It gets 4 pixels from the framebuffer.
        uint16_t *_fbPtr;

        // Calculate byte shift for each line.
        uint16_t _lineByteIncrement = SCREEN_WIDTH / (_pixelsPerByte * 2);

        // Get the 16 rows of the data (faster RAM read speed, since it reads whole RAM column at once).
        // Reading line by line will gets us only 89MB/s read speed, but reading 16 rows or more at once will get us
        // ~215MB/s read speed! Nice! Start the DMA transfer!
        HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_frameBuffer, (uint32_t)_oneLine1,
                          sizeof(_oneLine1), 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();
        _frameBuffer += sizeof(_oneLine1);

        // Set the current working RAM buffer to the first RAM Buffer (_oneLine1).
        _fbPtr = (uint16_t *)_oneLine1;

        // Decode the first line.
        _pixelDecode(_decodedLine1, _waveformLut, _fbPtr);
        _fbPtr += _lineByteIncrement;

        // Set the pointers for double buffering.
        _pendingDecodedLineBuffer = _decodedLine2;
        _currentDecodedLineBuffer = _decodedLine1;

        // Send to the screen!
        vScanStart();
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            hScanStart(_currentDecodedLineBuffer[0], _currentDecodedLineBuffer[1]);

            HAL_MDMA_Start_IT(_epdMdmaHandle, (uint32_t)_currentDecodedLineBuffer + 2,
                              (uint32_t)EPD_FMC_ADDR, sizeof(_decodedLine1), 1);

            // Decode the pixels into Waveform for EPD.
            (_pixelDecode)(_pendingDecodedLineBuffer, _waveformLut, _fbPtr);
            _fbPtr += _lineByteIncrement;

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
            while (stm32FmcEpdCompleteFlag() == 0)
                ;
            stm32FmcClearEpdCompleteFlag();

            // Advance the line on EPD.
            vScanEnd();

            // Check if the buffer needs to be updated (after 16 lines).
            if ((i & _prebufferedLines) == (_prebufferedLines - 1))
            {
                // Update the buffer pointer.
                _fbPtr = (uint16_t *)_oneLine1;

                // Start new RAM DMA transfer.
                HAL_MDMA_Start_IT(_sdramMdmaHandle, (uint32_t)_frameBuffer, (uint32_t)(_oneLine1), sizeof(_oneLine1),
                                  1);

                _frameBuffer += sizeof(_oneLine1);

                // Wait for DMA transfer to complete.
                while (stm32FmcSdramCompleteFlag() == 0)
                    ;
                stm32FmcClearSdramCompleteFlag();
            }
        }
}

/**
 * @brief   Static method used for covnverting framebuffer pixel data to data ready to be send to ePaper.
 * 
 * @param   void *_out
 *          Pointer to the locaton where to store decoded pixels.
 * @param   void *_lut
 *          Pointer to the location of the LUT used for decode.
 * @param   void *_fb
 *          Pointer to the location of the piel framebuffer.
 *          
 */
void EPDDriver::pixelDecode4BitEPD(void *_out, void *_lut, void *_fb)
{
    uint16_t *_fbHelper = (uint16_t*)_fb;
    for (int n = 0; n < (SCREEN_WIDTH / 4); n++)
    {
        ((uint8_t*)(_out))[n] = ((uint8_t*)(_lut))[*(_fbHelper++)];
    }
}

/**
 * @brief   Static method used for covnverting framebuffer pixel data to data ready to be send to ePaper.
 * 
 * @param   void *_out
 *          Pointer to the locaton where to store decoded pixels.
 * @param   void *_lut
 *          Pointer to the location of the LUT used for decode.
 * @param   void *_fb
 *          Pointer to the location of the piel framebuffer.
 *          
 */
void EPDDriver::pixelDecode1BitEPDFull(void *_out, void *_lut, void *_fb)
{
    uint8_t *_fbHelper = (uint8_t*)_fb;
    for (int n = 0; n < (SCREEN_WIDTH / 4); n += 2)
    {
        ((uint8_t*)(_out))[n] = ((uint8_t*)(_lut))[(*_fbHelper) >> 4];
        ((uint8_t*)(_out))[n + 1] = ((uint8_t*)(_lut))[(*(_fbHelper++)) & 0x0F];
    }
}

/**
 * @brief   Static method used for covnverting framebuffer pixel data to data ready to be send to ePaper.
 * 
 * @param   void *_out
 *          Pointer to the locaton where to store decoded pixels.
 * @param   void *_lut
 *          Pointer to the location of the LUT used for decode.
 * @param   void *_fb
 *          Pointer to the location of the piel framebuffer.
 *          
 */
void EPDDriver::pixelDecode1BitEPDPartial(void *_out, void *_lut, void *_fb)
{
    uint8_t *_fbHelper = (uint8_t*)_fb;
    memcpy(_out, _fbHelper, (SCREEN_WIDTH / 4));
}

#endif