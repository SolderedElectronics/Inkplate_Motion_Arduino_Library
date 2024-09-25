/**
 **************************************************
 *
 * @file        InkplateMotion.h
 * @brief       Main header file of the Inkplate library.
 *              Includes all necessary files into the library.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#include "InkplateMotion.h"

//--------------------------USER FUNCTIONS--------------------------------------------
Inkplate::Inkplate() : Adafruit_GFX(SCREEN_WIDTH, SCREEN_HEIGHT)
{
}

void Inkplate::begin(uint8_t _mode)
{
    INKPLATE_DEBUG_MGS("begin() started");

    // Check if the initializaton of the library already done.
    // In the case of already initialized library, return form the begin() funtion to
    // avoiid any memory leaks, multiple initializaton of the peripherals etc.
    if (_beginDone == 1)
        return;

    // Init I2C (Arduino Wire Library).
    Wire.setSCL(INKPLATE_TPS_SDA);
    Wire.setSCL(INKPLATE_TPS_SCL);
    Wire.begin();

// Force init. of the Serial if debug is used.
#ifdef __INKPLATE__DEBUG__
    // First, de-init the serial, in the case if the Serial is already initialized.
    Serial.end();

    // Init. serial communication at 115200 bauds.
    Serial.begin(115200);
#endif
    // Init low level driver for EPD.
    initDriver();

    // Start the library in selected mode. By default is 1bit mode.
    selectDisplayMode(_mode);

    // Set 16bit ADC resolution for battery measurement.
    analogReadResolution(16);

    // Clean frame buffers.
    clearDisplay();

    // Block multiple inits.
    _beginDone = 1;

    INKPLATE_DEBUG_MGS("begin() done");
}

// Draw function, used by Adafruit GFX.
void Inkplate::drawPixel(int16_t x0, int16_t y0, uint16_t color)
{
    if (x0 > width() - 1 || y0 > height() - 1 || x0 < 0 || y0 < 0)
        return;

    switch (rotation)
    {
    case 1:
        _swap_int16_t(x0, y0);
        x0 = height() - x0 - 1;
        break;
    case 2:
        x0 = width() - x0 - 1;
        y0 = height() - y0 - 1;
        break;
    case 3:
        _swap_int16_t(x0, y0);
        y0 = width() - y0 - 1;
        break;
    }

    if (getDisplayMode() == INKPLATE_1BW)
    {
        int x = x0 / 8;
        int xSub = x0 % 8;

        uint8_t temp = *(_pendingScreenFB + (SCREEN_WIDTH / 8 * y0) + x);
        *(_pendingScreenFB + (SCREEN_WIDTH / 8 * y0) + x) =
            ~pixelMaskLUT[xSub] & temp | (color ? pixelMaskLUT[xSub] : 0);
    }
    else
    {
        color &= 0x0f;
        int x = x0 / 2;
        int xSub = x0 % 2;
        uint8_t temp;

        temp = *(_pendingScreenFB + SCREEN_WIDTH / 2 * y0 + x);
        *(_pendingScreenFB + SCREEN_WIDTH / 2 * y0 + x) = pixelMaskGLUT1[xSub] & temp | (xSub ? color << 4 : color);
    }
}

void Inkplate::setRotation(uint8_t r)
{
    rotation = (r & 3);
    switch (rotation)
    {
    case 0:
    case 2:
        _width = SCREEN_WIDTH;
        _height = SCREEN_HEIGHT;
        break;
    case 1:
    case 3:
        _width = SCREEN_HEIGHT;
        _height = SCREEN_WIDTH;
        break;
    }
}

void Inkplate::drawBitmap4Bit(int16_t _x, int16_t _y, const unsigned char *_p, int16_t _w, int16_t _h)
{
    if (getDisplayMode() != INKPLATE_GL16)
        return;
    uint8_t _rem = _w % 2;
    int i, j;
    int xSize = _w / 2 + _rem;

    for (i = 0; i < _h; i++)
    {
        for (j = 0; j < xSize - 1; j++)
        {
            drawPixel((j * 2) + _x, i + _y, (*(_p + xSize * (i) + j) >> 4));
            drawPixel((j * 2) + 1 + _x, i + _y, (*(_p + xSize * (i) + j) & 0xff));
        }
        drawPixel((j * 2) + _x, i + _y, (*(_p + xSize * (i) + j) >> 4));
        if (_rem == 0)
            drawPixel((j * 2) + 1 + _x, i + _y, (*(_p + xSize * (i) + j) & 0xff));
    }
}