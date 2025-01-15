/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_WiFi_Simple.ino
 * @brief       Simply connect to your home Wi-Fi network and get some data
 *
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion library
#include <InkplateMotion.h>

Inkplate inkplate; // Create Inkplate object

// Change WiFi SSID and password here.
#define WIFI_SSID "Soldered"
#define WIFI_PASS "dasduino"

// Link to download text data from
char httpUrl[] = {"https://example.com/"};

void setup()
{
    inkplate.begin(INKPLATE_BLACKWHITE); // Initialize Inkplate in black and white mode

    // Set text options
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(2);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // Let's initialize the Wi-Fi library:
    WiFi.init();

    // Set mode to Station
    WiFi.setMode(INKPLATE_WIFI_MODE_STA);

    // Connect to WiFi:
    if (!WiFi.begin(WIFI_SSID, WIFI_PASS))
    {
        inkplate.println("Can't connect to Wi-Fi!");
        // Go to infinite loop
        while (true)
            delay(100);
    }
    inkplate.println("Successfully connected to Wi-Fi!");
    inkplate.display();

    // Let's now download a file
    // In this case, the HTML data of example.com will be downloaded

    // Create the WiFi client object for HTTP request
    WiFiClient client;
    // Since the file will be received in chunks, keeps track of the byte received
    uint32_t totalSize = 0;
    // Try to open the page
    if (client.begin(httpUrl))
    {
        if (client.GET())
        {
            inkplate.print("Connected, file size: ");
            inkplate.print(client.size(), DEC);
            inkplate.println("bytes");
            inkplate.partialUpdate(true);

            while (client.available())
            {
                // Use blocking method to get all chunks of the HTTP
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
        inkplate.partialUpdate(true);
    }
    else
    {
        inkplate.println("Failed to get the file");
        inkplate.partialUpdate(true);
    }
    // End client HTTP request
    // This is required! Otherwise any other AT command to the modem will fail
    client.end();

    // Print out the received bytes
    inkplate.print("\n\n\n\nTotal received bytes: ");
    inkplate.println(totalSize, DEC);

    // End message
    inkplate.println("Done!");
}

void loop()
{
}