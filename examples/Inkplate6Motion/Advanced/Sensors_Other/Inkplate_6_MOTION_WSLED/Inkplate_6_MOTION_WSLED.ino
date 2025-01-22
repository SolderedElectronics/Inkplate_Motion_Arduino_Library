/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_WSLED.ino
 * @brief       How to use the two onboard WS2812 LED's
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Library.
#include <InkplateMotion.h>


Inkplate inkplate; // Create object on Inkplate library

void setup()
{
    inkplate.begin();        // Init library
    inkplate.clearDisplay(); // Clear any data that may have been in (software) frame buffer
    inkplate.display();      // Clear the display

    // Power on LEDs
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WS_LED, true);

    // Init LEDs
    inkplate.led.begin();

    // Set brightness (0-255)
    inkplate.led.setBrightness(125);
}

void loop()
{
    // Cycle some colors on the two LEDs
    
    // Red
    inkplate.led.setPixelColor(0, 150, 0, 0);
    inkplate.led.setPixelColor(1, 150, 0, 0);
    inkplate.led.show();
    delay(500);
    // Green
    inkplate.led.setPixelColor(0, 0, 150, 0);
    inkplate.led.setPixelColor(1, 0, 150, 0);
    inkplate.led.show();
    delay(500);
    // Blue
    inkplate.led.setPixelColor(0, 0, 0, 150);
    inkplate.led.setPixelColor(1, 0, 0, 150);
    inkplate.led.show();
    delay(500);
    // White
    inkplate.led.setPixelColor(0, 150, 150, 150);
    inkplate.led.setPixelColor(1, 150, 150, 150);
    inkplate.led.show();
    delay(500);
}