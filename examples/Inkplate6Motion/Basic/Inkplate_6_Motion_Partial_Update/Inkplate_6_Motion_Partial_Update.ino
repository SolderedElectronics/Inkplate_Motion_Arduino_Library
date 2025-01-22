/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Partial_Update.ino
 * @brief       Example for using fast (partial) updates, avoid full screen refreshes
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion library
#include <InkplateMotion.h>

Inkplate inkplate; // Create Inkplate object

void setup()
{
    inkplate.begin(INKPLATE_BLACKWHITE); // Initialize Inkplate in black and white mode
    // Partial updates are explained below
}

void loop()
{
    // Let's slide text from left to right and do partial updates in black and white
    // These partial updates are the fastest on Inkplate 6MOTION
    inkplate.selectDisplayMode(INKPLATE_BLACKWHITE); // set display mode
    inkplate.clearDisplay();
    inkplate.display(); // Do full display to clear the screen
    delay(50);

    // Set text options
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK);
    inkplate.setTextWrap(false);

    int x = -500; // X coordinate, start from the left of the screen border
    while (x < 1024)
    {
        inkplate.clearDisplay();
        inkplate.setCursor(x, 300);
        inkplate.print("Partial updates in BW");
        inkplate.partialUpdate(true); // Do partial update while leaving the screen on
        x += 15;                      // Move 15 px to the right
    }
    inkplate.display(); // Do a full update to reset the display
    delay(1000);        // Wait a bit

    // Now let's demonstrate doing partial updates in grayscale
    // Doing this is less fast and more intended to reduce full-screen flickering in grayscale mode
    inkplate.selectDisplayMode(INKPLATE_GRAYSCALE);
    inkplate.clearDisplay();
    inkplate.display(); // Do full display to clear the screen
    // Set text options
    inkplate.setTextSize(3);
    inkplate.setTextColor(0);
    inkplate.setCursor(50, 50);
    inkplate.print("Partial updates in Grayscale");

    for (int repeat = 0; repeat < 8; repeat++)
    {
        x = 150; // Set x coordinate
        for (int i = 0; i <= 15; i++)
        {
            inkplate.fillCircle(x, 300, 100, i);
            x += 50; // Draw next circle with x offset
        }
        inkplate.partialUpdate(true);

        x = 150; // Reset x coordinate
        for (int i = 15; i >= 0; i--)
        {
            inkplate.fillCircle(x, 300, 100, i);
            x += 50; // Draw next circle with x offset
        }
        inkplate.partialUpdate(true);
    }

    inkplate.display(); // Do a full update to reset the display

    delay(1000); // Wait a bit
}