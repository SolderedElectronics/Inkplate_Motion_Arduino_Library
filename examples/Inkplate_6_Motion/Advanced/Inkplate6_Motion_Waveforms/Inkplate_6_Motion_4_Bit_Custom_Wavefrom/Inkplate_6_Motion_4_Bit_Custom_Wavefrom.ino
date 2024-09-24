// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Use provided wavefroms.
// #include "boards/Inkplate6Motion/waveforms.h"

// Create Inkplate Motion Object.
Inkplate inkplate;

// Ok, lets talk about these waveforms!
// Every waveform file consists of clean wavefrom and write waveform. Clear wavefrom is used for
// cleareing prev. image from the screen and write wavefrom for writing content on the screen.
// Writing 1 means this will darken the image, writing 0 means discharge, 2 menas to sends white
// pixel and 3 means to skip the pixel. Usually 3 is almost never used, 2 only on color change.
// The best is to go BLACK->WHITE->BLACK->WHITE.
// clang-format off
uint8_t customClearWavefrom4Bit[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};
// clang-format on

// Second thing that determ. wavefrom is the timing - how long one line write is holded. The longer, the better image
// quality, but slower refresh. This is only to some extend - too slow time can degrade image quality. This value is CPU
// cycles. There are two different timings - one for clear, other one for write. This timing is tuned to get 85Hz
// as eInk states.
uint32_t oneLineWriteClean = 550ULL;

// This is the write wavefrom - same as before; 0 = discharge, 1 = Black, 2 = White, 3 = Skip. But now this is
// two dimensional array, where each column represents one color (black to white from left to right) and rows each
// ePaper refresh phase.
// clang-format off
static uint8_t customWaveform4BitLUT[17][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 1, 1, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 0, 2, 2, 1, 1, 2},
    {1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2},
    {1, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
// clang-format on

// Same thing as prev. timing, only now is for writing pixels.
uint32_t oneLineWrite = 350ULL;

// Create the waveform struct.
static InkplateWaveform custom4BitWavefrom = {
    .mode = INKPLATE_WF_4BIT,
    .type = INKPLATE_WF_FULL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(customWaveform4BitLUT) / sizeof(customWaveform4BitLUT[0]),
    .lut = (uint8_t *)&(customWaveform4BitLUT[0]),
    .cycleDelay = oneLineWrite,
    .clearPhases = sizeof(customClearWavefrom4Bit) / sizeof(customClearWavefrom4Bit[0]),
    .clearLUT = customClearWavefrom4Bit,
    .clearCycleDelay = oneLineWriteClean,
    .name = "custom4BitFullUpdate",
};

void setup()
{
    // Initialize the serial communication for debugging.
    Serial.begin(115200);
    Serial.printf("Hello from %s\r\n", INKPLATE_BOARD_NAME);

    // Initialize Inkplate Moiton library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Set up the graphic engine for text.
    inkplate.setTextColor(0, 15);
    inkplate.setTextSize(2);

    // Draw the the colors on the screen.
    drawGradient(&inkplate, 0, 150, inkplate.width(), 100);

    // Draw something on the screen.
    drawScreen(&inkplate, "default");

    // Wait a little bit.
    delay(5000);

    // Now let use new custom wavefrom! It will be slower but the image quality should be improved.
    // First, remove everything from the framebuffer.
    inkplate.clearDisplay();

    // Load the new wavefrom!
    if (!inkplate.loadWaveform(custom4BitWavefrom))
    {
        Serial.println("Wavefrom load failed, halting");

        while (1)
            ;
    }
    // Draw the the colors on the screen.
    drawGradient(&inkplate, 0, 150, inkplate.width(), 100);

    // Now write the same thing but with new waveform.
    drawScreen(&inkplate, "custom");
}

void loop()
{
}

/**
 * @brief   Draws the preddefined text on the screen.
 * 
 * @param   Inkplate *_inkplate
 *          Inkplate Motion object pointer.
 * @param   const char *_wfType
 *          text to describe current used wavefrom (default or custom).
 *          
 */
void drawScreen(Inkplate *_inkplate, const char *_wfType)
{
    _inkplate->setCursor(100, 100);
    _inkplate->printf("4 Bit full update using %s wavefrom", _wfType);
    _inkplate->display();
}

/**
 * @brief   Draws color gradient on the screen in defined rectangle.
 * 
 * @param   Inkplate *_inkplate
 *          Inkplate Motion object pointer.
 * @param   int _x
 *          X start position of gradeint rectangle.
 * @param   int _y
 *          Y start position of gradeint rectangle.
 * @param   int _w
 *          Width of gradeint rectangle.
 * @param   int _h
 *          Height of gradeint rectangle.
 */
void drawGradient(Inkplate *_inkplate, int _x, int _y, int _w, int _h)
{
    // Calculate the lenght of each color (there are 16 colors available).
    int _xStep = _w / 16;

    // Draw each color.
    for (int i = 0; i < 16; i++)
    {
        _inkplate->fillRect((i * _xStep) + _x, _y, _xStep, _h, i);
    }
}