#ifndef __IPMOTION6DRIVER_H__
#define __IPMOTION6DRIVER_H__

// Header guard for the Arduino include
#ifdef BOARD_INKPLATE6_MOTION

// Inkplate Board name.
#define INKPLATE_BOARD_NAME "Inkplate 6 MOTION"

// Include main header file for the Arduino.
#include "Arduino.h"

// Include GPIO Pins definitions.
#include "pins.h"

// Include driver for the EPD PMIC.
#include "../../system/epdPmic/epdPmic.h"

// Include waveforms for EPD
#include "waveforms.h"

// Include library for the STM32 FMC
#include "../../stm32System/stm32FMC.h"

// Include library defines
#include "../../system/defines.h"

// Include library for PCAL6416A GPIO expander.
#include "../../system/PCAL_IO.h"

// Include the STM32 Helpers functions.
#include "../../system/helpers.h"

// Include file that defines each feature for each Inkplate board.
#include "../../features/featureSelect.h"

// Include RTC library for STM32H7.
#include "../../stm32System/STM32H7RTC.h"

// Defines for EPD GPIOs
#define EPD_DRIVER_PINS_OUTPUT  0
#define EPD_DRIVER_PINS_H_ZI    1

// FMC address for sending data to the EPD.
#define EPD_FMC_ADDR    0x68000000

// STM32 SPI for Inkplate System Stuff (WiFi & microSD).
static SPIClass _systemSpi(INKPLATE_MICROSD_SPI_MOSI, INKPLATE_MICROSD_SPI_MISO, INKPLATE_MICROSD_SPI_SCK);

// --- Functions declared static inline here for less calling overhead. ---
// Start writing the frame on the epaper display.
static inline void vScanStart()
{
    CKV_SET;
    delayMicroseconds(1);
    SPV_CLEAR;
    delayMicroseconds(6);
    CKV_CLEAR;
    delayMicroseconds(7);
    CKV_SET;
    delayMicroseconds(7);
    SPV_SET;
    delayMicroseconds(6);
    CKV_CLEAR;
    delayMicroseconds(1);
    CKV_SET;
    delayMicroseconds(10);
    CKV_CLEAR;
    delayMicroseconds(10);
    CKV_SET;
    delayMicroseconds(10);
    CKV_CLEAR;
    delayMicroseconds(10);
    CKV_SET;
    delayMicroseconds(10);
}

// Compiler be nice, please do not optimise this function.
__attribute__((optimize("O0"))) static inline void cycleDelay(uint32_t _cycles)
{
    while(_cycles--);
}

// Start writing the first line into epaper display.
__attribute__((always_inline)) static inline void hScanStart(uint8_t _d1, uint8_t _d2)
{
    *(__IO uint8_t *)(EPD_FMC_ADDR) = _d1;
    SPH_CLEAR;
    cycleDelay(5ULL);
    *(__IO uint8_t *)(EPD_FMC_ADDR) = _d1;
    CKV_SET;
    cycleDelay(150ULL);
    SPH_SET;
    *(__IO uint8_t *)(EPD_FMC_ADDR) = _d2;
}

// End writing the line into epaper display.
__attribute__((always_inline)) static inline void vScanEnd()
{
    CKV_CLEAR;
    cycleDelay(5ULL);
    LE_SET;
    *(__IO uint8_t *)(EPD_FMC_ADDR) = 0;
    cycleDelay(5ULL);
    LE_CLEAR;
    *(__IO uint8_t *)(EPD_FMC_ADDR) = 0;
    cycleDelay(5ULL);
}
// --- End of static inline declared functions. ---

class EPDDriver : public Helpers
{
    public:
        EPDDriver();
        int initDriver();
        void cleanFast(uint8_t *_clearWavefrom, uint8_t _wavefromPhases);
        void clearDisplay();
        void partialUpdate(uint8_t _leaveOn = 0);
        void partialUpdate4Bit(uint8_t _leaveOn);
        void display(uint8_t _leaveOn = 0);
        int epdPSU(uint8_t _state);
        bool loadWaveform(InkplateWaveform _customWaveform);
        double readBattery();
        void selectDisplayMode(uint8_t _mode);
        uint8_t getDisplayMode();

        // Set the automatic partial update.
        void setFullUpdateTreshold(uint16_t _numberOfPartialUpdates);

        // Should be moved into Inkplate.h or Graphics.h.
        void drawBitmapFast(const uint8_t *_p);

        // Initializer for microSD card.
        bool microSDCardInit();

        // Enable selected peripherals.
        void peripheralState(uint8_t _peripheral, bool _en);

        // Object for ePaper power managment IC.
        EpdPmic pmic;

        // Object for GPIO expander.
        IOExpander internalIO;

        // Object for the STM32 built-in RTC.
        STM32H7RTC rtc;

        // Object for the magnetic rotary encoder.
        AS5600 rotaryEncoder;

        // Object for NeoPixel LED.
        Adafruit_NeoPixel led = Adafruit_NeoPixel(2, INKPLATE_WSLED_DIN, NEO_GRB + NEO_KHZ800);

        // Object for SHTC3 temperature and humidity sensor.
        SHTC3 shtc3;

        // Object for the ADPS9960 Sensor.
        SparkFun_APDS9960 apds9960;

        // Object for LSM6DS3 accelerometer & gyroscope.
        LSM6DS3 lsm6ds3 = LSM6DS3(I2C_MODE, 0x6A);

        // Object for the Inkplate on-board micro SD card.
        SdFat sdFat;

    protected:
        // Function initializes all GPIO pins used on Inkplate for driving EPD.
        void gpioInit();

        // External SRAM frame buffers astart addresses. Statically allocated due speed.
        // Frame buffer for current image on the screen. 2MB in size (2097152 bytes).
        volatile uint8_t *_currentScreenFB = (uint8_t *)0xD0000000;

        // Frame buffer for the image that will be written to the screen on update. 2MB in size (2097152 bytes).
        volatile uint8_t *_pendingScreenFB = (uint8_t *)0xD0200000;

        // "Scratchpad memory" used for calculations (partial update for example). 2MB in size (2097152 bytes).
        volatile uint8_t *_scratchpadMemory = (uint8_t *)0xD0400000;

    private:
        // Sets EPD control GPIO pins to the output or High-Z state.
        void epdGpioState(uint8_t _state);

        // Function calculates 4 pixels at once from 4 bit per pixel buffer on the fly (before start writing new frame).
        // With this we can crate LUT that will take all 4 pixels from the frame buffer and convert them into waveform.
        void calculateGLUTOnTheFly(uint8_t *_lut, uint8_t *_waveform);

        // Function calculates the difference between tfo framebuffers (usually between current image on the screen and pending in the MCU memory).
        // Also returns the number of pixel that will be changed.
        uint32_t differenceMask(uint8_t *_currentScreenFB, uint8_t *_pendingScreenFB, uint8_t *_differenceMask);

        void display1b(uint8_t _leaveOn);
        
        void display4b(uint8_t _leaveOn);

        void pixelsUpdate(volatile uint8_t *_frameBuffer, uint8_t *_waveformLut, void (*_pixelDecode)(void*, void*, void*), const uint8_t _prebufferedLines, uint8_t _bitsPerPx);

        static void pixelDecode4BitEPD(void *_out, void *_lut, void *_fb);
        
        static void pixelDecode1BitEPDFull(void *_out, void *_lut, void *_fb);

        static void pixelDecode1BitEPDPartial(void *_out, void *_lut, void *_fb);

        // Object for the SdFat SPI STM32 library.
        SdSpiConfig* _microSDCardSPIConf = nullptr;

        // Typedef handles for Master DMA.
        MDMA_HandleTypeDef *_epdMdmaHandle;
        MDMA_HandleTypeDef *_sdramMdmaHandle;

        // Default EPD PSU state is off.
        uint8_t _epdPSUState = 0;

        // Current display mode. By defaul, set it to 1 bit B&W mode.
        uint8_t _displayMode = INKPLATE_1BW;
        
        // Block partial update at startup, use full update.
        uint8_t _blockPartial = 1;

        // Internal typedef for the 4 bit waveform - global update.
        InkplateWaveform _waveform4BitInternal = default4BitWavefrom;

        // Internal typedef for the 1 bit mode waveform - global update.
        InkplateWaveform _waveform1BitInternal = default1BitWavefrom;

        // Internal typedef for the 4 bit waveform - partial update.
        InkplateWaveform _waveform4BitPartialInternal = default4BitPartialUpdate;

        // Variable keeps track on how many partial updates have been executed for automatic full update.
        uint16_t _partialUpdateCounter = 0;

        // Variable that set the user-defined treshold for the full update. If zero, automatic full update is disabled.
        uint16_t _partialUpdateLimiter = 0;

        // Variable keeps current status of the microSD card initializaton.
        bool _microSdInit = false;

        // Fast LUT table for conversion from 2 * 4 bit grayscale pixel to EPD Wavefrom.
        uint8_t _fastGLUT[65536];
};

#endif

#endif