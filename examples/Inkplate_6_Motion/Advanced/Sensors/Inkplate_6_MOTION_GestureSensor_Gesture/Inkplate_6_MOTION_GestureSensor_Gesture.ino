/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_GestureSensor_Gesture.ino
 * @brief       This example will show you how to read simply read a detected gesture from the gesture sensor
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

// Include Inkplate Motion libary.
#include "InkplateMotion.h"

// Include Adafruit GFX Fonts.
#include "FreeSansBold24pt7b.h"

// Include the file containing bitmap of each gesture icon.
#include "icons.h"

// Include the background image
#include "image.h"

// Initialize Inkplate
Inkplate inkplate;

void setup()
{
    // Initialize Inkplate
    inkplate.begin();

    // Turn on the gesture sensor peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_APDS9960, true);

    // Do a full update each 15 partial ones
    inkplate.setFullUpdateTreshold(15);

    // Initialize APDS9960, notify user if init has failed
    if (!inkplate.apds9960.init())
    {
        inkplate.println("APDS-9960 initialization failed");
        inkplate.display();

        // Stop the code!
        while (1)
            ;
    }

    // Start running the APDS9960 gesture sensor engine
    // The parameter is to turn on interrupts on or off
    if (!inkplate.apds9960.enableGestureSensor(false))
    {
        // Print error message if failed
        Serial.println("Gesture sensor failed to start");
        inkplate.display();

        // Stop the code!
        while (1)
            ;
    }

    // Draw the background image for the example:
    inkplate.drawBitmap(0, 0, apdsBgImg, 1024, 768, BLACK);
    inkplate.setTextSize(4); // Set the text size for printing

    // Set the cursor in the appropriate position:
    inkplate.setCursor(450, 663);
    // Set the font for printingthe last gesture detected
    inkplate.setFont(&FreeSansBold24pt7b);
    inkplate.setTextSize(1);
    // Show everything on the display
    inkplate.display();
}

void loop()
{
    // Check if new gesture is detected.
    if (inkplate.apds9960.isGestureAvailable())
    {
        // Get the detected gesture and print the gesture on the screen.
        switch (inkplate.apds9960.readGesture())
        {
        case DIR_UP:
        // Add two spaces so it's more of a centered print
            inkplate.print("  UP");
            break;
        case DIR_DOWN:
            inkplate.print(" DOWN");
            break;
        case DIR_LEFT:
            inkplate.print(" LEFT");
            break;
        case DIR_RIGHT:
            inkplate.print("RIGHT");
            break;
        case DIR_NEAR:
            inkplate.print(" NEAR");
            break;
        case DIR_FAR:
            inkplate.print("  FAR");
            break;
        default:
            // If no gesture is detected, but something is detected, print out "?".
            inkplate.print("    ?");
        }

        // Quickly show on the display!
        inkplate.partialUpdate(false);

        // Now, erase what was previously written with a white fillRect
        inkplate.fillRect(298, 627, 440, 109, WHITE);
        inkplate.setCursor(450, 663); // Set the cursor back in it's original position
    }
}
