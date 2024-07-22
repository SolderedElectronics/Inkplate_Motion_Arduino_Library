/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_WiFi_Scan.ino
 * @brief       This example will show you how to scan available WiFi networks and print them
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

// Include the Inkplate Motion Library
#include <InkplateMotion.h>

// Create an Inkplate Motion Object
Inkplate inkplate;

// Setup code, runs only once
void setup()
{
    // Initialize the Inkplate Motion Library in 1bit mode
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Set text printing option
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // First enable the WiFi peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WIFI, true);

    // Here's some technical information on how WiFi works on Inkplate 6 MOTION:
    // The onboard ESP32 is a co-processor, it's connected to the STM32 via SPI
    // It's running a firmware called ESP-AT: https://github.com/espressif/esp-at
    // This means, it will perform WiFi functions sent to it and communicate back to the STM32
    // This is all done seamlessly through the Inkplate Motion library

    // Initialize ESP32 WiFi
    if (!WiFi.init())
    {
        // If we're here, couldn't initialize WiFi, something is wrong!
        inkplate.println("ESP32-C3 initialization Failed! Code stopped.");
        inkplate.display();
        // Go to infinite loop after informing the user
        while (1)
        {
            delay(100);
        }
    }
    // Great, initialization was successful
    inkplate.println("ESP32 WiFi module initialization OK!");
    inkplate.partialUpdate(true);

    // Set mode to station
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA))
    {
        inkplate.println("STA mode failed!");
        inkplate.partialUpdate(true);
        // Somehow this didn't work, go to infinite loop!
        while (1)
        {
            delay(100);
        }
    }

    // Scan the WiFi networks:
    inkplate.println("WiFi network scan:");
    inkplate.partialUpdate(true);
    int foundNetworks = WiFi.scanNetworks();
    // If networks are found...
    if (foundNetworks != 0)
    {
        inkplate.print("Networks found: ");
        inkplate.println(foundNetworks, DEC);

        // Print networks in order
        // In this print, RSSI is signal strength
        // -30 to -50 is considered excellent
        for (int i = 0; i < foundNetworks; i++)
        {
            inkplate.print(i + 1, DEC);
            inkplate.print(". RSSI: ");
            inkplate.print(WiFi.rssi(i), DEC);
            inkplate.print(' ');
            inkplate.print(WiFi.ssid(i));
            inkplate.println(WiFi.auth(i) ? '*' : ' ');
        }
    }
    else
    {
        // Couldn't find any networks!
        inkplate.println("No networks found.");
    }

    // Display the WiFi networks on screen
    inkplate.display();
}

void loop()
{
    // Do nothing for the rest of the code
}