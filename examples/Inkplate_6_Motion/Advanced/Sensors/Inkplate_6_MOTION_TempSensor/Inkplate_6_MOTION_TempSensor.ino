/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_TempSensor.ino
 * @brief       This example will show you how to read from the built in SHTC3 temperature sensor
 *
 *              To successfully run the sketch:
 *              -Connect Inkplate 6 MOTION via USB-C cable
 *              -Press the programming button to put the device in the programming state
 *              -Upload the code
 *
 * @see         solde.red/333321
 *
 * @authors     Robert @ soldered.com
 * @date        July 2024
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
        inkplate.setCursor(194, 547);
        inkplate.print(inkplate.shtc3.toDegC(), 2); // Get temperature and print
        inkplate.print(" C");                       // Print unit

        // Use toPercent to get the humidity% and print to display:
        inkplate.setCursor(650, 547);
        inkplate.print(inkplate.shtc3.toPercent(), 2); // Get humidity and print
        inkplate.print("%");                           // Print unit

        // Update the display
        inkplate.partialUpdate(true); 

        delay(1000); // Wait a longer while before taking the temperature again
    }
}