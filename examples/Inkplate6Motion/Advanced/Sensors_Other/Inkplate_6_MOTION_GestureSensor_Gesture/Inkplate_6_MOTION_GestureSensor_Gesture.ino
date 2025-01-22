/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_GestureSensor_Gesture.ino
 * @brief       Simply read detected gestures using the built in APDS9960 gesture sensor
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion libary
#include "InkplateMotion.h"

// Include Adafruit GFX Fonts
#include "FreeSansBold24pt7b.h"

// Include the background image
#include "image.h"

// Initialize Inkplate
Inkplate inkplate;

void setup()
{
    // Initialize Inkplate
    inkplate.begin(INKPLATE_BLACKWHITE);

    // Turn on the gesture sensor peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_APDS9960, true);

    // Do a full update each 15 partial updates automatically
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
    // Check if new gesture is detected periodically
    if (inkplate.apds9960.isGestureAvailable())
    {
        // Get the detected gesture and print the gesture on the screen
        switch (inkplate.apds9960.readGesture())
        {
        case DIR_UP:
            inkplate.print("  UP"); // Spaces are added for centered print
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
            // If no gesture is detected, but something is detected, print out "?"
            inkplate.print("    ?");
        }

        // Show on the display with a partial update
        inkplate.partialUpdate(false);

        // Now, erase what was previously written with a white fillRect
        inkplate.fillRect(298, 627, 440, 109, WHITE);
        inkplate.setCursor(450, 663); // Set the cursor back in it's original position
    }
    delay(10); // Slight delay between readings, helps with smoother detection
}
