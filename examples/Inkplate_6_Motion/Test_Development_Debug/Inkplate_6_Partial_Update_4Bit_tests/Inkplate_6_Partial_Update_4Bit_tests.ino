// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// // Dasduino boards.
// #include "image1.h"

// // My vinyl records.
// #include "image2.h"

// // My Audio system.
// #include "image3.h"

// // Vineyard Image - Inkplate Welcome Image
// #include "image4.h"

// Create an Inkplate Motion library object.
Inkplate inkplate;

int _colors[16];

// struct images
// {
//     int w;
//     int h;
//     const uint8_t *ptr;
// };

// struct images myImages[]
// {
//     {img1_w, img1_h, img1},
//     {img2_w, img2_h, img2},
//     {img3_w, img3_h, img3},
//     {img4_w, img4_h, img4},
// };

// int currentImage = 0;
// int maxImages = sizeof(myImages) / sizeof(myImages[0]);

void setup()
{
    // Initialize Inkplate Motion library.
    inkplate.begin(INKPLATE_GL16);

    Serial.begin(115200);

    // Draw a blank screen.
    inkplate.display();

    delay(2000);

    calculateColors(_colors, 0, 15);
    drawGrad(&inkplate, 0, 0, inkplate.width(), inkplate.height(), _colors, 16);
    inkplate.display();

    delay(2000);

    int _yStep = inkplate.height() / 16;
    for (int i = 0; i < 16; i++)
    {
        calculateColors(_colors, i, i + 15);
        // for (int j = 0; j < sizeof(_colors) / sizeof(_colors[0]); j++)
        // {
        //     Serial.print(_colors[j], DEC);
        //     Serial.print(" ");
        // }
        // Serial.println();
        drawGrad(&inkplate, 0, _yStep * i, inkplate.width(), _yStep, _colors, 16);
    }
    inkplate.partialUpdate();
    delay(2000);

    inkplate.clearDisplay();
    inkplate.partialUpdate();

    // Set the image change button to WAKE UP button.
    pinMode(PC13, INPUT);
}

void loop()
{
    // // Wait until the WAKE UP button is pressed.
    // while (digitalRead(PC13));

    // // Clear everything from the framebuffer.
    // inkplate.clearDisplay();

    // // Draw 4 bit image covnerted by using LCD image coverter (4bit Grayscale preset,
    // // 8bit packing instead of default 16bit). Image name must match included image!
    // // Ex. image7.h is included, img7, img7_w and imf7-h muist be used.
    // // Draw the image at the center of the screen.
    // inkplate.drawBitmap4Bit((inkplate.width() - myImages[currentImage].w) / 2, (inkplate.height() - myImages[currentImage].h) / 2, myImages[currentImage].ptr, myImages[currentImage].w, myImages[currentImage].h);
    
    // // Update the screen using partial updates.
    // inkplate.partialUpdate4Bit(true);

    // // Increment the image counter.
    // currentImage++;

    // // Check for the rollover.
    // if (currentImage >= maxImages) currentImage = 0;
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