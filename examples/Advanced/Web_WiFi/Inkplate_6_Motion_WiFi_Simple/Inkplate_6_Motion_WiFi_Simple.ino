/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_WiFi_Simple.ino
 * @brief       Simply connect to your home Wi-Fi network and GET some data
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

// Change WiFi SSID and password here
#define WIFI_SSID ""
#define WIFI_PASS ""

// Link to download text data from
char httpUrl[] = {"https://raw.githubusercontent.com/SolderedElectronics/Inkplate_Motion_Arduino_Library/refs/heads/"
                  "dev/examples/Advanced/Web_WiFi/Inkplate_6_Motion_WiFi_Simple/exampleFile.txt"};

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
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait until we're connected, this is strongly reccomended to do!
    // At least do delay(3000) to give the modem some time to connect
    inkplate.print("Connecting to Wi-Fi...");
    while (!WiFi.connected())
    {
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    inkplate.println("\nSuccessfully connected to Wi-Fi!");
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
        // Make GET request on that httpUrl
        if (client.GET())
        {
            // GET was successful! Let's print some info
            inkplate.print("Connected, file size: ");
            inkplate.print(client.size(), DEC);
            inkplate.println("bytes");
            inkplate.partialUpdate(true);
            inkplate.println("");
            inkplate.println("File contents:");

            // Read file contents:
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
    }
    else
    {
        inkplate.println("Failed to get the file!");
    }
    // Show updates on the display
    inkplate.partialUpdate(true);

    // End client HTTP request
    // This is required! Otherwise any other AT command to the modem will fail
    client.end();

    // All done!
}

void loop()
{
}