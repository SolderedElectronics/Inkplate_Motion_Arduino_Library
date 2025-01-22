/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_HTTP_POST.ino
 * @brief       Send some sensor data via HTTP POST request to webhook.site
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Add an Inkplate Motion Libray to the Sketch
#include <InkplateMotion.h>

// Include ArduinoJSON library. Get it from here: https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// Change WiFi SSID and password here
#define WIFI_SSID ""
#define WIFI_PASS ""

// Go to webhook.site and get a unique URL, something like:
// https://webhook.site/0e92fcdc-test-1234-test-44e5ca8a0b47
// Copy and paste it here, this is the URL where the POST request will be made:
char httpUrl[] = {""};
// On webhook.site, the post requests will be in visible in the sidebar to the left

// Create an Inkplate Motion Object
Inkplate inkplate;

void setup()
{
    // Initialize the Inkplate Motion Library
    inkplate.begin(INKPLATE_1BW);

    // Shuffle the seed to make random numbers
    randomSeed(analogRead(PA2) ^ analogRead(PA2));

    // Clear the screen
    inkplate.display();

    // Set the text
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
    // Show connection successful screen and keep it for a second
    inkplate.println("\nSuccessfully connected to Wi-Fi!");
    inkplate.display();
    delay(1000);

    // Clear the screen and print info message
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.println("Sending POST request each 30 seconds...\n\n");
    inkplate.partialUpdate(true);
}

void loop()
{
    // Let's periodically send the POST request with a random number:
    inkplate.println("-> Data sent, response:");
    inkplate.partialUpdate(true);
    sendPost();
    delay(30000); // Wait 30 seconds

    // Now let's do the same but with JSON
    inkplate.println("-> Data sent with JSON, response:");
    inkplate.partialUpdate(true);
    sendPostWithJson();
    delay(30000); // Wait 30 seconds
}

// This function sends a POST request to the set URL
void sendPost()
{
    // Create the WiFi client object for HTTP request
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received
    uint32_t totalSize = 0;

    // Begin connection to URL
    if (client.begin(httpUrl))
    {
        // Create request body, a simple request with one data field
        char bodyStr[256];
        sprintf(bodyStr, "data=%d", random(-127, 128)); // Add a random number from -126 to 128
        // Do POST
        if (client.POST(bodyStr, strlen(bodyStr)))
        {
            while (client.available())
            {
                // Use blocking method to get all chunks of the HTTP reply
                {
                    if (client.available() > 0)
                    {
                        char myBuffer[2000];
                        int n = client.read(myBuffer, sizeof(myBuffer));
                        totalSize += n;

                        // Print out the chunk
                        for (int i = 0; i < n; i++)
                        {
                            inkplate.print(myBuffer[i]);
                        }
                    }
                }
            }
        }
        // Add new line at the end
        inkplate.println();

        // Refresh the screen
        inkplate.partialUpdate(true);
    }
    else
    {
        // Error? inform the user
        inkplate.println("Failed to POST");
        inkplate.partialUpdate(true);
    }

    // End client HTTP request (MUST HAVE otherwise any other AT command to the modem will fail)
    if (!client.end())
    {
        inkplate.println("Client end problem");
    }
}

// This function sends the POST request but with a JSON body
void sendPostWithJson()
{
    // Create the WiFi client object for HTTP request
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received
    uint32_t totalSize = 0;

    // Connect to the URL
    if (client.begin(httpUrl))
    {
        // Add a HTTP header (must be added for JSON in this case)
        client.addHeader("Content-Type: application/json");

        // Create body with JSON
        char jsonData[256];
        StaticJsonDocument<200> webhookSiteJSON;
        // Add number data to field1
        webhookSiteJSON["dataField"] = random(-127, 128);
        // Serialize it so that it can be added to the POST request
        serializeJson(webhookSiteJSON, jsonData);

        // Do POST
        if (client.POST(jsonData, strlen(jsonData)))
        {
            while (client.available())
            {
                // Use blocking method to get all chunks of the HTTP reply
                {
                    if (client.available() > 0)
                    {
                        char myBuffer[2000];
                        int n = client.read(myBuffer, sizeof(myBuffer));
                        totalSize += n;

                        // Print out the chunk
                        for (int i = 0; i < n; i++)
                        {
                            inkplate.print(myBuffer[i]);
                        }
                    }
                }
            }
        }
        // Update the display
        inkplate.partialUpdate(true);
    }
    else
    {
        // Error? Inform the user
        inkplate.println("Failed to POST");
        inkplate.partialUpdate(true);
    }

    // End client HTTP request (MUST HAVE otherwise any other AT command to the modem will fail)
    if (!client.end())
    {
        inkplate.println("Client end problem");
    }
}