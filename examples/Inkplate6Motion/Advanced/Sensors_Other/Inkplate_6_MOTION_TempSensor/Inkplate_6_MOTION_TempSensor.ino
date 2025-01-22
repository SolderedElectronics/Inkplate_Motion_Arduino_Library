/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_TempSensor.ino
 * @brief       Measure temperature and humidity using the built-in SHTC3 sensor
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Include the background image
#include "image.h"

Inkplate inkplate; // Create object on Inkplate library

void setup()
{
    inkplate.begin();        // Init library
    inkplate.clearDisplay(); // Clear any data that may have been in (software) frame buffer
    inkplate.display();      // Clear the display

    // Set automatic full update after 30 partial updates
    inkplate.setFullUpdateTreshold(30);

    inkplate.shtc3.begin(); // Initialize sensor

    // Draw the background image for the example:
    inkplate.drawBitmap(0, 0, shtcBgImg, 1024, 758, BLACK);

    inkplate.setTextSize(4); // Set the text size for printing
}

// Loop code, this runs repeteadly
void loop()
{
    // Force SHTC3 sensor update. If update was successful, update the screen with new data
    // To get new data from the sensor, you have to run inkplate.shtc3.update()!
    if (inkplate.shtc3.update() == SHTC3_Status_Nominal)
    {
        // Let's clear what was previously printed with a white rectangle:
        inkplate.fillRect(92, 529, 840, 75, WHITE);

        // Use toDegC to read the temperature and print to display:
        // NOTE: SHTC3 requires a fixed calibration offset value
        // So your final temperature would be eg. inkplate.shtc3.toDegC() - 4.5
        // Measure one time in a temperature-controlled area and see what the offset value is
        inkplate.setCursor(194, 547);
        inkplate.print(inkplate.shtc3.toDegC(), 2); // Get temperature and print
        inkplate.print(" C");                       // Print unit

        // Use toPercent to get the humidity% and print to display:
        inkplate.setCursor(650, 547);
        inkplate.print(inkplate.shtc3.toPercent(), 2); // Get humidity and print
        inkplate.print("%");                           // Print unit

        // Update the display
        inkplate.partialUpdate(true);

        delay(50); // Wait bit inbetween refreshes
    }
}