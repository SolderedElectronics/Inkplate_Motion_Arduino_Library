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
// The main idea here is to set all prev. pixels to one uniform color, in this case all to white
// color.
// clang-format off
static uint8_t customWaveform4BitPartialLUTClean[11][16] = 
{
    {2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
};
// clang-format on

// Second thing that determ. wavefrom is the timing - how long one line write is holded. The longer, the better image
// quality, but slower refresh. This is only to some extend - too slow time can degrade image quality. This value is CPU
// cycles. There are two different timings - one for clear, other one for write. This timing is tuned to get 85Hz
// as eInk states.
uint32_t oneLineWriteClean = 200ULL;

// This is the write wavefrom - same as before; 0 = discharge, 1 = Black, 2 = White, 3 = Skip. But now this is
// two dimensional array, where each column represents one color (black to white from left to right) and rows each
// ePaper refresh phase.
// clang-format off
static uint8_t customWaveform4BitPartialLUT[17][16] = 
{
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 1, 1, 2, 2},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 0, 2, 2, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2},
    {1, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
// clang-format on

// Same thing as prev. timing, only now is for writing pixels.
uint32_t oneLineWrite = 200ULL;

// Create the waveform struct.
static InkplateWaveform custom4BitWavefrom = {
    .mode = INKPLATE_WF_4BIT,
    .type = INKPLATE_WF_PARTIAL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(customWaveform4BitPartialLUT) / sizeof(customWaveform4BitPartialLUT[0]),
    .lut = (uint8_t *)&(customWaveform4BitPartialLUT[0]),
    .cycleDelay = oneLineWrite,
    .clearPhases = sizeof(customWaveform4BitPartialLUTClean) / sizeof(customWaveform4BitPartialLUTClean[0]),
    .clearLUT = (uint8_t*)(&customWaveform4BitPartialLUTClean[0]),
    .clearCycleDelay = oneLineWriteClean,
    .name = "custom4BitPartialUpdate",
};

void setup()
{
    // Initialize the serial communication for debugging.
    Serial.begin(115200);
    Serial.printf("Hello from %s\r\n", INKPLATE_BOARD_NAME);

    // Initialize Inkplate Moiton library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Draw example screen with the partial updates on 4 bit with default wavefrom.
    drawGraphics(&inkplate, "default");

    // Wait a little bit.
    delay(5000);

    // Now let use new custom wavefrom! It will be slower but the image quality should be improved.
    // First, remove everything from the framebuffer.
    inkplate.clearDisplay();

    // Load the new wavefrom!
    if (!inkplate.loadWaveform(custom4BitWavefrom))
    {
        Serial.println("Wavefrom load failed, halting");

        while(1);
    }

    // Draw example screen with the partial updates on 4 bit with default wavefrom.
    drawGraphics(&inkplate, "custom");
}

void loop()
{

}

void drawScreen(Inkplate *_inkplate, const char *_wfType)
{
        
    // Set up the graphic engine for text.
    _inkplate->setTextSize(2);
    _inkplate->setTextColor(0, 15);

    // Write text on the screen.
    _inkplate->setCursor(100, 100);
    _inkplate->printf("4 Bit partial update using %s wavefrom", _wfType);
    _inkplate->display();
}

void drawGrad(Inkplate *_display, int _x, int _y, int _w, int _h, int *_colors, int _nColors)
{
    int _xStep = _w / _nColors;
    for (int i = 0; i < _nColors; i++)
    {
        _display->fillRect(_x + (i * _xStep), _y, _xStep, _h, _colors[i]);
    }
}

void calculateColors(int *_colors, int _startColor, int _endColor)
{
    int _numberOfColors = abs(_startColor - _endColor) + 1;
    
    for (int i = 0; i < _numberOfColors; i++)
    {
        _colors[i] = (_startColor + i) % 16;
    }
}

void drawGraphics(Inkplate *_inkplate, const char *_wfType)
{
    int _colors[16];
    drawScreen(_inkplate, _wfType);
    delay(2500);

    calculateColors(_colors, 0, 15);
    drawGrad(_inkplate, 0, 0, inkplate.width(), inkplate.height(), _colors, 16);
    _inkplate->partialUpdate();

    // Wait a little bit.
    delay(2000);

    int _yStep = inkplate.height() / 16;
    for (int i = 0; i < 16; i++)
    {
        calculateColors(_colors, i, i + 15);
        drawGrad(&inkplate, 0, _yStep * i, inkplate.width(), _yStep, _colors, 16);
    }
    _inkplate->partialUpdate();
}