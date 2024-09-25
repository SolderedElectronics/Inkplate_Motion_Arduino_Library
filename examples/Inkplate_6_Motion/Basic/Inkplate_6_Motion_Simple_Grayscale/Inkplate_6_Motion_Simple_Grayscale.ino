// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Select one of provided images. Do not forget to update the image name in drawBitmap4Bit!
//#include "image1.h" (Marina's picture).

// Road.
//#include "image2.h"

// Dasduino boards.
//#include "image3.h"

// Picrute of the tree (Marina's picture).
//#include "image4.h"

// My vinyl records.
//#include "image5.h"

// My Audio system.
//#include "image6.h"

// Robert image - Grayscale (stairs).
//#include "image7.h"

// Robert image - Grayscale (buildings).
//#include "image8.h"

// Robert image - Anime1
//#include "image9.h"

// Robert image - Anime2
//#include "image10.h"

// Vineyard Image - Inkplate Welcome Image
#include "image11.h"

// Create an Inkplate Motion library object.
Inkplate inkplate;

void setup()
{
    // Initialize Inkplate Motion library.
    inkplate.begin(INKPLATE_GL16);

    // Draw gradient lines on epaper showing all possible colors.
    drawGrad(&inkplate, 0, 300, inkplate.width(), 300);
    // Update the screen.
    inkplate.display();
    // Wait a little bit.
    delay(5000);

    // Clear everything from the framebuffer.
    inkplate.clearDisplay();
    // Draw 4 bit image covnerted by using LCD image coverter (4bit Grayscale preset,
    // 8bit packing instead of default 16bit). Image name must match included image!
    // Ex. image7.h is included, img7, img7_w and imf7-h muist be used.
    // Draw the image at the center of the screen.
    inkplate.drawBitmap4Bit((inkplate.width() - img11_w) / 2, (inkplate.height() - img11_h) / 2, img11, img11_w, img11_h);
    // Update the screen.
    inkplate.display();
}

void loop()
{
    // Empty, nothing to look here.
}

void drawGrad(Inkplate *_inkplate, int _x, int _y, int _w, int _h)
{
    // Calculate the step for x cursor.
    int _xStep = _w / 16;

    // Draw each color.
    for (int i = 0; i < 16; i++)
    {
        _inkplate->fillRect(_x + (_xStep * i), _y, _xStep, _h, i);
    }
}