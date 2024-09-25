// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Use provided wavefroms if needed.
//#include "boards/Inkplate6Motion/waveforms.h"

// Create Inkplate Motion Object.
Inkplate inkplate;

// Second thing that determ. wavefrom is the timing - how long one line write is holded. The longer, the better image quality, but
// slower refresh. This is only to some extend - too slow time can degrade image quality. This value is CPU cycles. There are two
// different timings - one for clear, other one for write. This timing is tuned to get 85Hz as eInk states.
uint32_t oneLineWriteClean = 550ULL;

// Same thing as prev. timing, only now is for writing pixels.
uint32_t oneLineWrite = 750ULL;

// Create the waveform struct. The only parameters that can be adjusted in 1 bit partial updates is
// number of write phases and line write delay. That's it! In this case, nuber of write phases is set to 6.
InkplateWaveform custom1BitWavefrom = 
{
    .mode = INKPLATE_WF_1BIT,
    .type = INKPLATE_WF_PARTIAL_UPDATE,
    .tag = 0xef,
    .lutPhases = 9,
    .lut = nullptr,
    .cycleDelay = oneLineWrite,
    .clearPhases = 0,
    .clearLUT = nullptr,
    .clearCycleDelay = 0,
    .name = "custom1BitPartial",
};

void setup()
{
    // Initialize the serial communication for debugging.
    Serial.begin(115200);
    Serial.printf("Hello from %s\r\n", INKPLATE_BOARD_NAME);

    // Initialize Inkplate Moiton library in 1 bit mode.
    inkplate.begin(INKPLATE_1BW);

    // Draw something on the screen.
    drawScreen(&inkplate, "default");

    // Draw the counter from 0 to 99 on the screen.
    drawNumbers(&inkplate, 200, 200);

    // Wait a little bit.
    delay(5000);

    // Now let use new custom wavefrom! It will be slower but the image quality should be improved.
    // First, remove everything from the framebuffer. And also do a full update.
    inkplate.clearDisplay();
    inkplate.display();

    // Load the new wavefrom!
    if (!inkplate.loadWaveform(custom1BitWavefrom))
    {
        Serial.println("Wavefrom load failed, halting");

        while(1);
    }

    // Now write the same thing but with new waveform.
    drawScreen(&inkplate, "custom");

    // Draw the counter from 0 to 99 on the screen.
    drawNumbers(&inkplate, 200, 200);
}

void loop()
{

}

void drawScreen(Inkplate *_inkplate, const char *_wfType)
{
        
    // Set up the graphic engine for text.
    _inkplate->setTextSize(2);
    _inkplate->setTextColor(BLACK, WHITE);

    // Write text on the screen.
    _inkplate->setCursor(100, 100);
    _inkplate->printf("1 Bit partial update using %s wavefrom", _wfType);
    _inkplate->display();
}

void drawNumbers(Inkplate *_inkplate, int _x, int _y)
{
    // Number for increment.
    int _number = 0;

    // Set large text scaling.
    _inkplate->setTextSize(30);

    // Draw the text fast as possible.
    for (int i = 0; i < 100; i++)
    {
        // Set text cursor position
        _inkplate->setCursor(_x, _y);

        // Write the number!
        _inkplate->setTextColor(BLACK, WHITE);
        _inkplate->println(_number, DEC);
        _inkplate->setTextColor(WHITE, BLACK);
        _inkplate->setCursor(_x, _inkplate->getCursorY());
        _inkplate->println(_number++, DEC);

        // Do a partial update with ePaper PSU on.
        _inkplate->partialUpdate(true);
    }
}