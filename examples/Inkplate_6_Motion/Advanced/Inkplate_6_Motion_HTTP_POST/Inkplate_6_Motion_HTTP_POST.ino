// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Include ArduinoJSON library. Get it from here: https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// Change WiFi SSID and password here.
#define WIFI_SSID "Soldered-testingPurposes"
#define WIFI_PASS "Testing443"

// HTTP link to the POST method of the thingspeak channel (for x-www-form-urlencoded).
char httpUrl[] = {"https://api.thingspeak.com/update"};

// HTTP link for the POST method of the thingspeak channel (for json).
char httpUrlJson[] = {"https://api.thingspeak.com/update.json"};

#define THINGSPEAK_API_KEY "GEP5UQH43795ZIZG"

// Create an Inkplate Motion Object.
Inkplate inkplate;

void setup()
{
    // Setup a Serial communication for debug at 115200 bauds.
    Serial.begin(115200);

    // Print an welcome message (to know if the Inkplate board and STM32 are alive).
    Serial.println("Inkplate Motion Code Started!");

    // Initialize the Inkplate Motion Library.
    inkplate.begin(INKPLATE_1BW);

    // Shuffle the seed for the random.
    randomSeed(analogRead(PA2) ^ analogRead(PA2));

    // Clear the screen.
    inkplate.display();

    // Set the text.
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(2);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // First enable the WiFi peripheral.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WIFI, true);

    // Initialize ESP32 AT Commands Over SPI library.
    if (!WiFi.init())
    {
        inkplate.println("ESP32-C3 initializaiton Failed! Code stopped.");
        inkplate.partialUpdate(true);

        while (1)
        {
            delay(100);
        }
    }
    inkplate.println("ESP32 Initialization OK!");
    inkplate.partialUpdate(true);

    // If using static IP or different DNS, you can set it here. Parameters that you want to keep default
    // use INADDR_NONE as parameter.
    // WiFi.config(IPAddress(192, 168, 71, 3), IPAddress(192, 168, 71, 1), IPAddress(255, 255, 255, 0), IPAddress(8, 8,
    // 8, 8), IPAddress(8, 8, 4, 4));

    // Set it back to the station mode.
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA))
    {
        inkplate.println("STA mode failed!");
        inkplate.partialUpdate(true);

        while (1)
        {
            delay(100);
        }
    }

    // Scan the WiFi networks.
    inkplate.println("WiFi network scan:");
    inkplate.partialUpdate(true);

    int foundNetworks = WiFi.scanNetworks();
    if (foundNetworks != 0)
    {
        inkplate.print("Networks found: ");
        inkplate.println(foundNetworks, DEC);

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
        inkplate.println("No networks found.");
    }
    inkplate.partialUpdate(true);

    // Connect to the WiFi network.
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.print(" wifi...");
    inkplate.partialUpdate(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.connected())
    {
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    inkplate.println("connected!");
    inkplate.partialUpdate(true);

    // Prtint out IP config data.
    inkplate.print("Local IP: ");
    inkplate.println(WiFi.localIP());
    inkplate.partialUpdate(true);

    // Keep content on the screen for few more moments.
    delay(1000);

    // Clear the screen for the HTTP response.
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.println("Open https://thingspeak.com/channels/2568110. Cool-down time between each send is 30 sec.\n");
    inkplate.partialUpdate(true);
}

void loop()
{
    inkplate.println("-> Data sent, response:");
    inkplate.partialUpdate(true);
    sendPost();
    delay(30000);

    inkplate.println("-> Data sent with JSON, response:");
    inkplate.partialUpdate(true);
    sendPostWithJson();
    delay(30000);
}

void sendPost()
{
    // Create the WiFi client object for HTTP request.
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received.
    uint32_t totalSize = 0;

    // Try to open a web page.
    if (client.begin(httpUrl))
    {
        // Create body.
        char bodyStr[256];
        sprintf(bodyStr, "api_key=%s&field1=%d", THINGSPEAK_API_KEY, random(-127, 128));

        if (client.POST(bodyStr, strlen(bodyStr)))
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

        // Refresh the screen.
        inkplate.partialUpdate(true);
    }
    else
    {
        inkplate.println("Failed to POST");
        inkplate.partialUpdate(true);
    }

    // End client HTTP request (MUST HAVE otherwise any other AT command to the modem will fail).
    if (!client.end())
    {
        inkplate.println("Client end problem");
    }
}

void sendPostWithJson()
{
    // Create the WiFi client object for HTTP request.
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received.
    uint32_t totalSize = 0;

    // Try to open a web page.
    if (client.begin(httpUrlJson))
    {
        // Add a HTTP header (must be added for JSON in this case).
        client.addHeader("Content-Type: application/json");

        // Create body with JSON.
        char jsonData[256];
        StaticJsonDocument<200> thingspeakJson;
        thingspeakJson["api_key"] = THINGSPEAK_API_KEY;
        thingspeakJson["field1"] = random(-127, 128);
        serializeJson(thingspeakJson, jsonData);

        if (client.POST(jsonData, strlen(jsonData)))
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
        inkplate.partialUpdate(true);
    }
    else
    {
        inkplate.println("Failed to POST");
        inkplate.partialUpdate(true);
    }

    // End client HTTP request (MUST HAVE otherwise any other AT command to the modem will fail).
    if (!client.end())
    {
        inkplate.println("Client end problem");
    }
}