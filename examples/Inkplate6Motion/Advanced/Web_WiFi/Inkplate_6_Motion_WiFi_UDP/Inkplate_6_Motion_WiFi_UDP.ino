/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_WiFi_UDP.ino
 * @brief       Connect to your home Wi-Fi and get time data from UDP server
 *
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Add an Inkplate Motion Libray to the Sketch
#include <InkplateMotion.h>

// Change WiFi SSID and password here
#define WIFI_SSID ""
#define WIFI_PASS ""

// Create an Inkplate Motion Object
Inkplate inkplate;

// Use Google public NTP server
// You can change it for some other NTP server (time.windows.com, time.apple.com, etc)
// Or you can use the IP address 169.254.169.123 (Amazon NTP server)
const char ntpServerName[] = {"time.google.com"};

// Set the timezone in hours as well
// You can use negative numbers (eg. -8 for Los Angeles)
int timezone = 2;

void setup()
{
    // Initialize the Inkplate Motion Library
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Set the text options
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

    // Keep content on the screen for few more moments
    delay(1000);

    // Clear the screen for the HTTP response
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.print("Getting current time using NTP from ");
    inkplate.print(ntpServerName);
    inkplate.println("...");
    inkplate.partialUpdate(true);
}

void loop()
{
    // Try to get the NTP time from selected NTP server
    time_t epoch = getUDPClock();

    // If there is some epoch data (epoch is not zero), swe gotr something
    if (epoch)
    {
        // Add timezone to it
        epoch += (timezone * 3600);

        // Convert it into human readable time and date
        struct tm ntpTime;
        memcpy(&ntpTime, localtime((const time_t *)(&epoch)), sizeof(ntpTime));

        // Print the time
        char timeAndDateStr[40];
        sprintf(timeAndDateStr, "NTP: %02d:%02d:%02d %d.%02d.%04d. %+dGMT", ntpTime.tm_hour, ntpTime.tm_min,
                ntpTime.tm_sec, ntpTime.tm_mday, ntpTime.tm_mon + 1, ntpTime.tm_year + 1900, timezone);
        inkplate.println(timeAndDateStr);
        inkplate.partialUpdate(false);
    }
    else
    {
        // Print error if NTP read failed
        inkplate.println("NTP clock getting failed!");
        inkplate.partialUpdate(false);
    }

    // Wait half a minute before new time sync (you don't need to do this often)
    delay(30000ULL);
}

time_t getUDPClock()
{
    // Return value (epoch time)
    time_t _epoch = 0;

    // Create object for the UDP protcol
    WiFiUDP udp;

    // Set connection timeout for 10 seconds
    udp.setConnectionTimeout(10000ULL);

    // Initialize UDP. Uze port 8888 for NTP local port
    if (udp.begin(8888))
    {
        // Try to connect to the host. NTP uses port 123. Return 0 if failed
        if (udp.setHost(ntpServerName, 123))
        {
            // Initialize UDP for packet sending
            udp.beginPacket();

            // Create the byte array for the NTP
            uint8_t ntpPacket[48];
            // Clock is unsync, NTP version 4, Symmetric passive
            ntpPacket[0] = B11100011;
            // Stratum, or type of clock
            ntpPacket[1] = 0;
            // Polling Interval
            ntpPacket[2] = 6;
            // Peer Clock Precision
            ntpPacket[3] = 0xEC;
            // 8 bytes of zero for Root Delay & Root Dispersion
            ntpPacket[12] = 49;
            ntpPacket[13] = 0x4E;
            ntpPacket[14] = 49;
            ntpPacket[15] = 52;

            // Send the packet! NTP packet is 48 bytes long
            if (udp.write(ntpPacket, 48))
            {
                // Wait for the response
                if (udp.available())
                {
                    // Read the response
                    udp.read(ntpPacket, 48);

                    // Extract NTP epoch from received data
                    int32_t epochRaw = ntpPacket[40] << 24 | ntpPacket[41] << 16 | ntpPacket[42] << 8 | ntpPacket[43];
                    _epoch = epochRaw - 2208988800UL;
                }
            }
        }
    }

    // Must be called! Otherwise any future command to the ESP32 can fail
    udp.end();

    // Return the value of the epoch. Zero means request has failed
    return _epoch;
}