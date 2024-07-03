/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_HTTP_GET.ino
 * @brief       This example will show you how to make a GET request via WiFi
 *
 *              To successfully run the sketch:
 *              -Enter WiFi data below
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

// Add WiFi data here
#define WIFI_SSID ""
#define WIFI_PASS ""

// The URL which we're making GET to
#define GET_URL "http://www.example.com"
// The http:// part is important in this case!
// (As we're only getting the html data)
// HTTP and HTTPS both work fine

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

    // Connect to the WiFi network.
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.print("...");
    inkplate.partialUpdate(true);
    // This is the function which connects to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait until connected
    while (!WiFi.connected())
    {
        // Print a dot for each second we're waiting
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    // Great, we're connected! Inform the user
    inkplate.println("connected!");
    inkplate.partialUpdate(true);
    delay(1000); // Wait a bit

    // Inform the user about making the GET request
    inkplate.print("Making GET request to ");
    inkplate.print(GET_URL);
    inkplate.println("...");
    inkplate.partialUpdate(true);
    delay(1000); // Wait a bit

    // Make the text smaller so printing out the website fits
    inkplate.setTextSize(2);

    // Do GET and print the results, details in this function
    makeGet();

    // Display the results of the makeGet function
    inkplate.display();
}

void loop()
{
    // Do nothing for the rest of the code
}

// Simple function to make GET request and print the results to Inkplate
void makeGet()
{
    // Create the WiFi client object for HTTP request.
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received.
    uint32_t totalSize = 0;

    // Try to open a web page.
    if (client.begin(GET_URL))
    {
        if (client.GET())
        {
            while (client.available())
            {
                // Use blocking method to get all chunks of the HTTP.
                {
                    if (client.available() > 0)
                    {
                        char myBuffer[2000];
                        int n = client.read(myBuffer, sizeof(myBuffer));
                        totalSize += n;

                        // Print out the chunk.
                        for (int i = 0; i < n; i++)
                        {
                            inkplate.print(myBuffer[i]);
                        }
                    }
                }
            }
        }
        // Add new line at the end.
        inkplate.println();
        // Screen will be refreshed outside this function
    }
    else
    {
        inkplate.println("Failed to make GET request!");
        // Screen will be refreshed outside this function
    }

    // IMPORTANT!
    // End client HTTP request
    // This is essential, otherwise any other AT command to the modem after will fail!
    client.end();
}
