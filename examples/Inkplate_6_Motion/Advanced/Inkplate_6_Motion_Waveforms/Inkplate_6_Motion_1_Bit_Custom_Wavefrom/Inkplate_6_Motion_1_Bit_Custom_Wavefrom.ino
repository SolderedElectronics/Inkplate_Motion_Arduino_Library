// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Use provided wavefroms if needed.
//#include "boards/Inkplate6Motion/waveforms.h"

// Create Inkplate Motion Object.
Inkplate inkplate;

// Ok, lets talk about these waveforms!
// Every waveform file consists of clean wavefrom and write waveform. Clear wavefrom
// is used for cleareing prev. image from the screen and write wavefrom for writing content on the screen.
// Writing 1 means this will darken the image, writing 0 means discharge, 2 menas to sends white
// pixel and 3 means to skip the pixel. Usually 3 is almost never used, 2 only on color change.
// The best is to go BLACK->WHITE->BLACK->WHITE.

uint8_t customClearWavefrom1Bit[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

// Second thing that determ. wavefrom is the timing - how long one line write is holded. The longer, the better image quality, but
// slower refresh. This is only to some extend - too slow time can degrade image quality. This value is CPU cycles. There are two
// different timings - one for clear, other one for write. This timing is tuned to get 85Hz as eInk states.
uint32_t oneLineWriteClean = 550ULL;

// This is the write wavefrom - it will choose how to write new black pixels on the screen.
// In this case it will darken all pixels set as 1 in frame buffer. You can create yout own
// LUTs for that. LUTB represends every possible pixel combinaton in which each pixel is darken
// (except in the last phase).
static uint8_t *customWavefrom1BitLUT[] = {LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTBW, LUTD};

// Same thing as prev. timing, only now is for writing pixels.
uint32_t oneLineWrite = 750ULL;

// Create the waveform struct.
InkplateWaveform custom1BitWavefrom = 
{
    .mode = INKPLATE_WF_1BIT,
    .type = INKPLATE_WF_FULL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(customWavefrom1BitLUT) / sizeof(customWavefrom1BitLUT[0]),
    .lut = (uint8_t*)&(customWavefrom1BitLUT[0]),
    .cycleDelay = oneLineWrite,
    .clearPhases = sizeof(customClearWavefrom1Bit) / sizeof(customClearWavefrom1Bit[0]),
    .clearLUT = customClearWavefrom1Bit,
    .clearCycleDelay = oneLineWriteClean,
    .name = "custom1BitFullUpdate",
};

void setup()
{
    // Initialize the serial communication for debugging.
    Serial.begin(115200);
    Serial.printf("Hello from %s\r\n", INKPLATE_BOARD_NAME);

    // Initialize Inkplate Moiton library in 1 bit mode.
    inkplate.begin(INKPLATE_1BW);

    // Set up the graphic engine for text.
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextSize(2);

    // Draw something on the screen.
    drawScreen(&inkplate, "default");

    // Wait a little bit.
    delay(5000);

    // Now let use new custom wavefrom! It will be slower but the image quality should be improved.
    // First, remove everything from the framebuffer.
    inkplate.clearDisplay();

    // Load the new wavefrom!
    if (!inkplate.loadWaveform(custom1BitWavefrom))
    {
        Serial.println("Wavefrom load failed, halting");

        while(1);
    }

    // Now write the same thing but with new waveform.
    drawScreen(&inkplate, "custom");
}

void loop()
{

}

void drawScreen(Inkplate *_inkplate, const char *_wfType)
{
    _inkplate->fillRect(200, 200, 450, 200, BLACK);
    _inkplate->setCursor(100, 100);
    _inkplate->printf("1 Bit full update using %s wavefrom", _wfType);
    _inkplate->display();
}