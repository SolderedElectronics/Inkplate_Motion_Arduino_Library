/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_NTP.ino
 * @brief       This example will show you how to get time via NTP server
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

// Use Google public NTP server.
// You can change it for some other NTP server (time.windows.com, time.apple.com, etc)
// Or you can use the IP address 169.254.169.123 (Amazon NTP server)
#define NTP_SERVER "time.google.com"

// How long to wait between requests (seconds)
#define DELAY_S 30

// Define timezone (+ or - in hours)
int timezone = 2;

// Create an Inkplate Motion Object
Inkplate inkplate;

// Setup code, runs only once
void setup()
{
    // Initialize the Inkplate Motion Library in 1-bit mode
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Set the text printing options
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // Initialize ESP32 WiFi
    if (!WiFi.init())
    {
        inkplate.println("ESP32-C3 initialization Failed! Code stopped.");
        inkplate.partialUpdate(false);

        while (1)
        {
            delay(100);
        }
    }
    inkplate.println("ESP32 Initialization OK!");
    inkplate.partialUpdate(false);

    // Set mode to station
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA))
    {
        inkplate.println("STA mode failed!");
        inkplate.partialUpdate(false);

        while (1)
        {
            delay(100);
        }
    }

    // Connect to the WiFi network
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.print("...");
    inkplate.partialUpdate(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.connected())
    {
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    inkplate.println("connected!");
    inkplate.partialUpdate(false);

    // Let's initialize the RTC
    // This way, the recieved time data can be stored in the RTC
    // You could for example get the time once per day from NTP and store it in RTC
    // This is demonstrated in this sketch in loop()
    inkplate.println("Initializing RTC...");
    inkplate.partialUpdate(false);
    inkplate.rtc.begin(RTC_HOURFORMAT_24);
    delay(1000); // Wait a bit

    // Clear the screen and let's display the results:
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.print("Getting current time using NTP from ");
    inkplate.print(NTP_SERVER);
    inkplate.println("...\n");
    inkplate.partialUpdate(false);
}

void loop()
{
    // Try to get the NTP time from the selected NTP server
    time_t epoch = getUDPClock();

    // If there is some epoch data (epoch is not zero), we got something
    if (epoch)
    {
        // Add timezone to it
        epoch += (timezone * 3600);

        // Convert it into human-readable time and date
        struct tm ntpTime;
        memcpy(&ntpTime, localtime((const time_t *)(&epoch)), sizeof(ntpTime));

        // Print the time and date
        char timeAndDateStr[40];
        sprintf(timeAndDateStr, "NTP: %02d:%02d:%02d %d.%02d.%04d %+dGMT", ntpTime.tm_hour, ntpTime.tm_min,
                ntpTime.tm_sec, ntpTime.tm_mday, ntpTime.tm_mon + 1, ntpTime.tm_year + 1900, timezone);
        inkplate.print(timeAndDateStr);

        // Let's also set this time to the RTC
        inkplate.rtc.setTime(ntpTime.tm_hour, ntpTime.tm_min, ntpTime.tm_sec, 0);
        // Set the date (weekday is optional, can be set to 0 if not needed)
        inkplate.rtc.setDate(ntpTime.tm_mday, ntpTime.tm_mon + 1, ntpTime.tm_year + 1900, ntpTime.tm_wday);
        // You can use  if (!inkplate.rtc.isTimeSet()) to check if time is set

        // Let's now also print time from RTC:
        printRTC();

        inkplate.println(""); // Print newline
    }
    else
    {
        // Print error if NTP read failed
        inkplate.println("NTP clock getting failed!");
    }

    // Do partial update to update what's on the display
    inkplate.partialUpdate(false);

    // Wait for the set number of seconds
    delay(DELAY_S * 1000);
}

// Function to get time via NTP using UDP protocol
time_t getUDPClock()
{
    // Return value (epoch time)
    time_t _epoch = 0;

    // Create object for the UDP protocol
    WiFiUDP udp;

    // Set connection timeout for 10 seconds
    udp.setConnectionTimeout(10000ULL);

    // Initialize UDP. Use port 8888 for NTP local port
    if (udp.begin(8888))
    {
        // Try to connect to the host. NTP uses port 123. Return 0 if failed
        if (udp.setHost(NTP_SERVER, 123))
        {
            // Initialize UDP for packet sending
            udp.beginPacket();

            // Create the byte array for the NTP
            uint8_t ntpPacket[48] = {0};
            // Set up NTP request packet
            ntpPacket[0] = 0b11100011; // LI, Version, Mode
            ntpPacket[1] = 0;          // Stratum, or type of clock
            ntpPacket[2] = 6;          // Polling Interval
            ntpPacket[3] = 0xEC;       // Peer Clock Precision
            // Root Delay & Root Dispersion
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

// Function to print data via RTC
void printRTC()
{
    // Variables for time and date
    uint8_t h, m, s, d, mn, y, wk;
    uint32_t ss;

    // Get time and date data from STM32 internal RTC using pointers
    inkplate.rtc.getTime(&h, &m, &s, &ss, NULL, NULL);
    inkplate.rtc.getDate(&d, &mn, &y, &wk);

    // Print out the time and date on the Inkplate
    inkplate.println();
    inkplate.print("RTC Time: ");
    inkplate.print(h, DEC);
    inkplate.print(':');
    printWithLeadingZeros(m, 2);
    inkplate.print(':');
    printWithLeadingZeros(s, 2);
    inkplate.print("  Date: ");
    printWithLeadingZeros(d, 2);
    inkplate.print('.');
    printWithLeadingZeros(mn, 2);
    inkplate.print('.');
    inkplate.print(y + 2000, DEC);
}

// Helper function to print number with leading zeros
void printWithLeadingZeros(uint32_t _number, int _leadingZeros)
{
    for (int i = (_leadingZeros - 1); i >= 0; i--)
    {
        uint32_t _divider = pow(10, i);
        inkplate.print((_number / _divider) % 10, DEC);
    }
}