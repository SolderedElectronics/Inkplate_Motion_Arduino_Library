/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_GestureSensor_Other.ino
 * @brief       Read ambient light, color and proximity values from the built-in APDS9960 sensor
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

// Initialize Inkplate
Inkplate inkplate;

void setup()
{
    // Initialize Inkplate
    inkplate.begin();

    // Do a full update each 60 partial ones
    inkplate.setFullUpdateTreshold(60);

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

    // Enable sensor functionalities
    inkplate.apds9960.enableLightSensor(false);     // Enable light sensor with no interrupts
    inkplate.apds9960.enableProximitySensor(false); // Enable proximity sensor with no interrupts

    // Set the font for printing for this example
    inkplate.setFont(&FreeSansBold24pt7b);
}

void loop()
{
    // Variables for sensor data
    uint16_t ambient_light = 0;
    uint16_t red_light = 0, green_light = 0, blue_light = 0;
    uint8_t proximity_data = 0;

    // Clear what was previously in the display buffer
    inkplate.clearDisplay();
    inkplate.setCursor(50, 50);
    inkplate.print("APDS9960 Sensor readings:");

    // Read ambient light and color data
    if (inkplate.apds9960.readAmbientLight(ambient_light) && inkplate.apds9960.readRedLight(red_light) &&
        inkplate.apds9960.readGreenLight(green_light) && inkplate.apds9960.readBlueLight(blue_light))
    {
        // Display ambient light and color sensor data
        inkplate.setCursor(50, 150);
        inkplate.printf("Ambient: %d | R: %d G: %d B: %d", ambient_light, red_light, green_light, blue_light);
    }
    else
    {
        inkplate.setCursor(50, 150);
        inkplate.println("Error reading light values");
    }

    // Read proximity data
    if (inkplate.apds9960.readProximity(proximity_data))
    {
        // Display proximity data
        inkplate.setCursor(50, 250);
        inkplate.printf("Proximity: %d", proximity_data);
    }
    else
    {
        inkplate.setCursor(50, 250);
        inkplate.println("Error reading proximity");
    }

    // Update display with partial update
    inkplate.partialUpdate();
    delay(10); // Update every 10ms
}