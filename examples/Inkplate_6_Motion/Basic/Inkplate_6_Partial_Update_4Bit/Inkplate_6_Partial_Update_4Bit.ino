// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Dasduino boards.
#include "image3.h"

// My vinyl records.
#include "image5.h"

// My Audio system.
#include "image6.h"

// Vineyard Image - Inkplate Welcome Image
#include "image7.h"

// Create an Inkplate Motion library object.
Inkplate inkplate;

struct images
{
    int w;
    int h;
    const uint8_t *ptr;
};

struct images myImages[]
{
    {img3_w, img3_h, img3},
    {img5_w, img5_h, img5},
    {img6_w, img6_h, img6},
    {img7_w, img7_h, img7},
};

int currentImage = 0;
int maxImages = sizeof(myImages) / sizeof(myImages[0]);

void setup()
{
    // Initialize Inkplate Motion library.
    inkplate.begin(INKPLATE_GL16);

    // Draw a blank screen.
    inkplate.display();

    // Set the image change button to WAKE UP button.
    pinMode(PC13, INPUT);
}

void loop()
{
    // Wait until the WAKE UP button is pressed.
    while (digitalRead(PC13));

    // Clear everything from the framebuffer.
    inkplate.clearDisplay();

    // Draw 4 bit image covnerted by using LCD image coverter (4bit Grayscale preset,
    // 8bit packing instead of default 16bit). Image name must match included image!
    // Ex. image7.h is included, img7, img7_w and imf7-h muist be used.
    // Draw the image at the center of the screen.
    inkplate.drawBitmap4Bit((inkplate.width() - myImages[currentImage].w) / 2, (inkplate.height() - myImages[currentImage].h) / 2, myImages[currentImage].ptr, myImages[currentImage].w, myImages[currentImage].h);
    
    // Update the screen using partial updates.
    inkplate.partialUpdate(true);

    // Increment the image counter.
    currentImage++;

    // Check for the rollover.
    if (currentImage >= maxImages) currentImage = 0;
}