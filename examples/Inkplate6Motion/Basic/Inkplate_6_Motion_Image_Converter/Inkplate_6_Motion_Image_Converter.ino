/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Image_Converter.ino
 * @brief       Example for drawing images using the Soldered Image Converter
 *
 * Download the image converter at: https://github.com/SolderedElectronics/Soldered-Image-Converter
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion library
#include <InkplateMotion.h>

// Include converted images, make sure to use 1-bit mode in conversion
#include "images/image1.h"
#include "images/image2.h"
#include "images/image3.h"
#include "images/image4.h"

Inkplate inkplate; // Create Inkplate object

void setup()
{
    inkplate.begin(INKPLATE_BLACKWHITE); // Initialize Inkplate in black and white mode
}

void loop()
{
    // At the beginning of the loop, re-initialize the display mode as black and white:
    inkplate.selectDisplayMode(INKPLATE_BLACKWHITE);
    inkplate.clearDisplay();
    inkplate.display(); // Do full display to clear the screen

    // Draw the first image at 0, 0
    // It's full screen size, so it will cover the entire screen
    // The last parameter determines the color of the bitmap
    // The image is pre-dithered using Floyd-Steinberg in the image converter
    inkplate.drawBitmap(0, 0, image1, image1_w, image1_h, BLACK);
    inkplate.display(); // Update the display
    delay(5000);        // Wait for 5s so the image can be seen

    // For the next image, let's draw the Soldered logo
    // Monochrome bitmaps can also be printed in white pixels, by setting the last parameter to white
    // The image is not dithered in the image converter to retain the sharpness
    inkplate.fillRect(0, 0, inkplate.width(), inkplate.height(),
                      BLACK);                                         // fill the screen black so the image can be seen
    inkplate.drawBitmap(100, 100, image2, image2_w, image2_h, WHITE); // Draw the bitmap in white
    inkplate.display();                                               // Show it on the display

    delay(5000); // Wait for 5s so the image can be seen

    // Let's switch display modes to grayscale and draw a pre-dithered grayscale image
    inkplate.selectDisplayMode(INKPLATE_GRAYSCALE);
    inkplate.clearDisplay();
    inkplate.display(); // Do full display to clear the screen

    // To draw grayscale images, use drawBitmap4Bit
    // When converting, 4 bit mode and Floyd-Steinberg dithering were used
    // This allows for best image quality on Inkplate 6MOTION
    inkplate.drawBitmap4Bit(0, 0, image3, image3_w, image3_h);
    inkplate.display(); // Show it on the display

    delay(5000); // Wait for 5s so the image can be seen

    // Finally, let's clear the screen and draw a gradient
    inkplate.clearDisplay(); // Clear the display first

    // The gradient was converted in 4 bit mode but not dithered, so the gray levels can be observed
    // Draw it at 112, 479 so it's centered
    inkplate.drawBitmap4Bit(112, 279, image4, image4_w, image4_h);
    inkplate.display(); // Show it on the display

    delay(5000); // Wait for 5s so the image can be seen
}