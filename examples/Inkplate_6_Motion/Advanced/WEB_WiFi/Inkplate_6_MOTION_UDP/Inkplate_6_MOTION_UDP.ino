/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_UDP.ino
 * @brief       This example will show you how to get and send data to your computer via UDP
 *
 *              To successfully run the sketch:
 *              -Find out your computer's local IP address
 *              -Enter WiFi data, IP address data port data below
 *              -Upload the sketch on Inkplate so you find out Inkplate's IP address
 *              -Open the udp_chat.py Python script and modify the IP address there
 *              -Run udp_chat.py
 *              -Open serial monitor at 115200 baud and send messages, this will send messages via UDP
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

// Define UDP settings
#define LOCAL_PORT  123 // This port worked on our network
#define REMOTE_PORT 123
#define REMOTE_IP   "192.168.0.0" // Change this to the IP address of your computer

// Create an Inkplate Motion Object
Inkplate inkplate;

// Create a UDP object
WiFiUDP udp;

void setup()
{
    // Start serial communication, it's needed for this example
    Serial.begin(115200);

    // Initialize the Inkplate Motion Library in 1-bit mode
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Set the text printing options
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // First enable the WiFi peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WIFI, true);

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
    // Also print the local IP so it's visible to the user
    inkplate.print("The IP address of Inkplate is: ");
    inkplate.println(WiFi.localIP());
    inkplate.partialUpdate(false);

    // Initialize UDP
    udp.begin(LOCAL_PORT);

    Serial.println("UDP chat service started");
}

void loop()
{
    // Check for incoming serial data
    if (Serial.available() > 0)
    {
        // Received data? Send it on UDP
        String message = Serial.readStringUntil('\n');
        sendMessage(message);
    }

    // Check for incoming UDP packets
    int packetSize = udp.available();
    if (packetSize)
    {
        // Create buffer and read the data in it
        uint8_t incomingPacket[packetSize + 1];
        udp.read(incomingPacket, packetSize);
        incomingPacket[packetSize] = '\0'; // Null-terminate the string

        // Print received message on ePaper
        inkplate.print("Received: ");
        inkplate.println((char *)incomingPacket);
        inkplate.partialUpdate(false);
    }
}

// Simple function to send a message via UDP
// This is a bit slow because the host gets set and end-ed all within the function
void sendMessage(String message)
{
    udp.setHost(REMOTE_IP, REMOTE_PORT);
    udp.beginPacket();
    udp.write((uint8_t *)message.c_str(), message.length());
    udp.end();
    inkplate.print("Sent: ");
    inkplate.println(message);
    inkplate.partialUpdate(false);
}