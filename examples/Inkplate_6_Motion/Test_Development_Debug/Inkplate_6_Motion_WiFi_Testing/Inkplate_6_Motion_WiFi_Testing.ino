// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Change WiFi SSID and password here.
#define WIFI_SSID   "Soldered-testingPurposes"
#define WIFI_PASS   "Testing443"

// Select one of the HTTP links.
char httpUrl[] = {"https://raw.githubusercontent.com/BornaBiro/Inkplate6NextGen/development-updated-SDRAM/examples/Basic/Inkplate_6_Motion_Simple_WiFi_Demo/Inkplate_6_Motion_Simple_WiFi_Demo.ino"};
// char httpUrl[] = {"https://raw.githubusercontent.com/BornaBiro/ESP32-C3-SPI-AT-Commands/main/lorem_ipsum_long.txt"};
// char httpUrl[] = {"https://raw.githubusercontent.com/BornaBiro/ESP32-C3-SPI-AT-Commands/main/lorem_ipsum.txt"};

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

    // Clear the screen.
    inkplate.display();

    // Set the text.
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(2);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

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

    // If using static IP or different DNS, you can set it here. Parameters that you want to keep default, use
    // INADDR_NONE as parameter.
    // WiFi.config(IPAddress(192, 168, 71, 3), IPAddress(192, 168, 71, 1), IPAddress(255, 255, 255, 0), IPAddress(8, 8,
    // 8, 8), IPAddress(8, 8, 4, 4));

    // Set to Access Point to change the MAC Address.
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_AP))
    {
        inkplate.println("AP mode failed!");
        inkplate.partialUpdate(true);

        // No point going on with the code it we can't set WiFi mode.
        while (1)
        {
            delay(100);
        }
    }

    // Print out ESP32 MAC address.
    inkplate.print("ESP32 MAC Address: ");
    inkplate.println(WiFi.macAddress());
    inkplate.partialUpdate(true);

    // Change MAC address to something else. Only can be used with SoftAP!
    if (!WiFi.macAddress("1a:bb:cc:01:23:45"))
    {
        inkplate.println("MAC address Change failed!");
        inkplate.partialUpdate(true);
    }
    else
    {
        inkplate.print("New MAC address: ");
        inkplate.println(WiFi.macAddress());
        inkplate.partialUpdate(true);
    }

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

    inkplate.print("Gateway: ");
    inkplate.println(WiFi.gatewayIP());

    inkplate.print("Subnet mask: ");
    inkplate.println(WiFi.subnetMask());
    inkplate.partialUpdate(true);

    for (int i = 0; i < 3; i++)
    {
        inkplate.print("DNS ");
        inkplate.print(i, DEC);
        inkplate.print(": ");
        inkplate.println(WiFi.dns(i));
    }
    inkplate.partialUpdate(true);

    // Keep content on the screen for few more seconds.
    delay(1000);

    openPage();

    pinMode(PC13, INPUT);
}

void loop()
{
    if (!digitalRead(PC13)) openPage();
}

void openPage()
{
        // Clear the screen for the HTTP response.
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.println("Trying to open a text file from the internet...");
    inkplate.partialUpdate(true);

    // Create the WiFi client object for HTTP request.
    WiFiClient client;

    // Since the file will be received in chunks, keeps track of the byte received.
    uint32_t totalSize = 0;

    // Try to open a web page.
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
        inkplate.println("Failed to get the file");
        inkplate.partialUpdate(true);
    }

    // End client HTTP request (MUST HAVE otherwise any other AT command to the modem will fail).
    if (!client.end())
    {
        inkplate.println("Client end problem");
    }

    // Print out the received bytes.
    inkplate.print("\n\n\n\nTotal received bytes: ");
    inkplate.println(totalSize, DEC);

    // End message.
    inkplate.println("Done!");
}
