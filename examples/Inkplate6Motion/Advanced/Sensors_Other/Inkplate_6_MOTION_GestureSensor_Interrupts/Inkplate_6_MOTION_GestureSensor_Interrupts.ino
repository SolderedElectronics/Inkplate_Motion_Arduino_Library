/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_GestureSensor_Interrupts.ino
 * @brief       Detect gesture and read via interrupt
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

// ISR flag - Automatically set to true in case of Interrupt event from the IO Expander
volatile bool isrFlag = false;

// ISR handler function - Called when IO expander fires INT
void ioExpanderISR()
{
    isrFlag = true;
}

void setup()
{
    // Initialize Inkplate
    inkplate.begin();

    // Do a full update each 15 partial ones
    inkplate.setFullUpdateTreshold(15);

    // Let's enable interrupts
    // Set APDS INT pin on IO Expander as input. Override any GPIO pin protection
    inkplate.internalIO.pinModeIO(IO_PIN_A0, INPUT, true);
    // Set interrupts on IO expander
    inkplate.internalIO.setIntPinIO(IO_PIN_A0);
    // Enable interrptus on STM32
    // NOTE: Must be set to CHANGE!
    attachInterrupt(digitalPinToInterrupt(PG13), ioExpanderISR, CHANGE);

    // Turn on the gesture sensor peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_APDS9960, true);
    delay(100); // Wait a bit

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
    // Turn them on for this example
    if (!inkplate.apds9960.enableGestureSensor(true))
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
    // If the ISR flag was set...
    if (isrFlag)
    {
        // Check if the INT pin for the APDS9960 is set to low. Otherwise ignore the INT event
        // (must be set to low for INT event from APDS). Override any GPIO pin protection
        if (!inkplate.internalIO.digitalReadIO(IO_PIN_A0, true))
        {

            // Check if new gesture is detected
            if (inkplate.apds9960.isGestureAvailable())
            {
                // Get the detected gesture and print the gesture on the screen
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
                    // If no gesture is detected, but something is detected, print out "?"
                    inkplate.print("    ?");
                }

                // Quickly show on the display!
                inkplate.partialUpdate(false);

                // Now, erase what was previously written with a white fillRect
                inkplate.fillRect(298, 627, 440, 109, WHITE);
                inkplate.setCursor(450, 663); // Set the cursor back in it's original position
            }
        }
        // Clear the flag so that new interrupt event can be detected
        isrFlag = false;
    }
}
