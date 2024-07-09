// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Change WiFi SSID and password here.
#define WIFI_SSID "Soldered-testingPurposes"
#define WIFI_PASS "Testing443"

// Create an Inkplate Motion Object.
Inkplate inkplate;

// Use Google public NTP server.
// You can change it for some other NTP server (time.windows.com, time.apple.com, etc)
// Or you can use the IP address 169.254.169.123 (Amazon NTP server)
const char ntpServerName[] = {"time.google.com"};

// Set the timezone in hours as well.
int timezone = 2;

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
    inkplate.print("Getting current time using NTP from the ");
    inkplate.print(ntpServerName);
    inkplate.println("...");
    inkplate.partialUpdate(true);
}

void loop()
{
    // Try to get the NTP time from selected NTP server.
    time_t epoch = getUDPClock();

    // If there is some epoch data (epoch is not zero), swe gotr something.
    if (epoch)
    {
        // Add timezone to it.
        epoch += (timezone * 3600);

        // Convert it into human readable time and date.
        struct tm ntpTime;
        memcpy(&ntpTime, localtime((const time_t*)(&epoch)), sizeof(ntpTime));
        
        // Print the time.
        char timeAndDateStr[40];
        sprintf(timeAndDateStr, "NTP: %02d:%02d:%02d %d.%02d.%04d. %+dGMT", ntpTime.tm_hour, ntpTime.tm_min, ntpTime.tm_sec, ntpTime.tm_mday, ntpTime.tm_mon + 1, ntpTime.tm_year + 1900, timezone);
        inkplate.println(timeAndDateStr);
        inkplate.partialUpdate(false);
    }
    else
    {
        // Print error if NTP read failed.
        inkplate.println("NTP clock getting failed!");
        inkplate.partialUpdate(false);
    }

    // Wait half a minute before new time sync (you don't need to do this often).
    delay(30000ULL);
}

time_t getUDPClock()
{
    // Return value (epoch time).
    time_t _epoch = 0;

    // Create object for the UDP protcol.
    WiFiUDP udp;

    // Set connection timeout for 10 seconds.
    udp.setConnectionTimeout(10000ULL);

    // Initialize UDP. Uze port 8888 for NTP local port.
    if (udp.begin(8888))
    {
        // Try to connect to the host. NTP uses port 123. Return 0 if failed.
        if (udp.setHost(ntpServerName, 123))
        {
            // Initialize UDP for packet sending.
            udp.beginPacket();

            // Create the byte array for the NTP.
            uint8_t ntpPacket[48];
            // Clock is unsync, NTP version 4, Symmetric passive.
            ntpPacket[0] = B11100011;
            // Stratum, or type of clock.
            ntpPacket[1] = 0;
            // Polling Interval.
            ntpPacket[2] = 6;
            // Peer Clock Precision.
            ntpPacket[3] = 0xEC;
            // 8 bytes of zero for Root Delay & Root Dispersion
            ntpPacket[12]  = 49;
            ntpPacket[13]  = 0x4E;
            ntpPacket[14]  = 49;
            ntpPacket[15]  = 52;

            // Send the packet! NTP packet is 48 bytes long.
            if (udp.write(ntpPacket, 48))
            {
                // Wait for the response.
                if (udp.available())
                {
                    // Read the response.
                    udp.read(ntpPacket, 48);

                    // Extract NTP epoch from received data.
                    int32_t epochRaw = ntpPacket[40] << 24 | ntpPacket[41] << 16 | ntpPacket[42] << 8 | ntpPacket[43];
                    _epoch = epochRaw - 2208988800UL;
                }
            }
        }
    }

    // Must be called! Otherwise any future command to the ESP32 can fail.
    udp.end();

    // Return the value of the epoch. Zero means request has failed.
    return _epoch;
}