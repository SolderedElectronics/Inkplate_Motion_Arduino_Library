/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_WiFi_MQTT.ino
 * @brief       How to use MQTT on Inkplate 6MOTION
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Change WiFi SSID and password here.
#define WIFI_SSID ""
#define WIFI_PASS ""

// Create an Inkplate Motion Object.
Inkplate inkplate;

// Create the MQTT object.
// NOTE: Only one MQTT broker connection can be used even if multiple objects are created.
WiFiMQTT mqtt;

// Define the topic to transfer to, and topic to recieve from
const char mqttTopicTx[] = "solderedTest/solderedInkplate";
const char mqttTopicRx[] = "solderedTest/solderedDasduino";

// The MQTT server name
#define MQTT_SERVER "test.mosquitto.org"

// Variables for storing button state to be able to detect state change.
bool publishBtnOldState = true;
bool disconnectBtnOldState = true;
bool unsubBtnOldState = true;

void setup()
{
    // Initialize the Inkplate Motion Library.
    inkplate.begin(INKPLATE_1BW);

    // Set text scaling.
    inkplate.setTextSize(2);

    // Use WAKE/USER1 pin for publishing MQTT data to the INPUT_PULLUP.
    pinMode(PC13, INPUT_PULLUP);
    // Use USER2 pin for the MQTT disconnect to the INPUT_PULLUP.
    pinMode(PG6, INPUT_PULLUP);
    // Use USER3 pin for the MQTT unsubscribe from the topic to the INPUT_PULLUP.
    pinMode(PA0, INPUT_PULLUP);

    // Clear the screen.
    inkplate.display();

    // First enable the WiFi peripheral.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WIFI, true);

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

    // Keep content on the screen for few more seconds.
    delay(1000);

    // Initialize MQTT library and use internal dynalically buffer of
    // 48 bytes for MQTT RX messages. Do the init. after WiFi connection.
    mqtt.begin(48);

    // Set the MQTT server data.
    mqtt.setServer(MQTT_SERVER, 1883);

    // Try to connect to the MQTT broker. There is no ClientID, Username and Password.
    inkplate.print("Connecting to the MQTT broker...");
    inkplate.partialUpdate(true);
    if (!mqtt.connect())
    {
        inkplate.println("Connect failed!");
        inkplate.partialUpdate(false);

        // Do not go any further if failed.
        while (1)
            ;
    }
    inkplate.println("Connected!");
    inkplate.partialUpdate(true);

    // Try to subscribe to topic.
    inkplate.print("Subcribing to ");
    inkplate.print(mqttTopicRx);
    inkplate.print("topic...");
    inkplate.partialUpdate(true);
    if (!mqtt.subscribe((char *)mqttTopicRx))
    {
        inkplate.print("failed!");
        inkplate.partialUpdate(false);

        // Do not go any further if failed.
        while (1)
            ;
    }
    inkplate.println("subscribed!\r\n");
    inkplate.partialUpdate(true);

    // Delay a little bit.
    delay(1000);

    // Make the screen ready for MQTT traffic.
    inkplate.clearDisplay();
    inkplate.setCursor(0, 0);
    inkplate.println("Buttons: WAKE - Publish data, USER1 - Disconnect, USER2 - Unsubscribe\nMQTT traffic:");
    inkplate.display();
}

// Timer variable used for printing connection status every few seconds.
unsigned long timer1 = 0;

void loop()
{
    // Must be in the loop to periodically check recived data.
    // Also if HTTP or any other ESP32 functionallity, data loss can occur.
    mqtt.loop();

    // Check if there is any new data available on the topic.
    int mqttData = mqtt.available();
    if (mqttData)
    {
        // If so, print is out!
        inkplate.print("Topic: ");
        inkplate.print(mqtt.topic());
        inkplate.print(", Payload: ");
        for (int i = 0; i < mqttData; i++)
        {
            inkplate.print(mqtt.read());
        }
        inkplate.println();
        inkplate.partialUpdate(false);
    }

    // Check the state of the pin. If the pin is pressed, send new MQTT message.
    if (checkButton(PC13, &publishBtnOldState, LOW))
    {
        // Buffer for the MQTT message of 40 bytes.
        char message[40];

        // Make a MQTT message/payload.
        sprintf(message, "inkplate_%lu", millis());

        // Send the message/payload and display the success/fail.
        inkplate.print("Publish \"");
        inkplate.print(message);
        inkplate.print("\" ");
        if (mqtt.publish((char *)mqttTopicTx, message, 0, true))
        {
            inkplate.println("ok");
        }
        else
        {
            inkplate.println("failed");
        }
        inkplate.partialUpdate(false);
    }

    // If the USER2 button is pressed, disconnect from the MQTT broker.
    if (checkButton(PG6, &disconnectBtnOldState, LOW))
    {
        inkplate.print("Disconnect ");
        if (mqtt.disconnect())
        {
            inkplate.println("ok");
        }
        else
        {
            inkplate.println("fail");
        }
        inkplate.partialUpdate(false);
    }

    // If the USER2 button is pressed, unsubscribe from the topic.
    if (checkButton(PA0, &disconnectBtnOldState, LOW))
    {
        inkplate.print("Unsubscribe from topic...");
        if (mqtt.unsubscribe((char *)mqttTopicRx))
        {
            inkplate.println("ok");
        }
        else
        {
            inkplate.println("fail");
        }
        inkplate.partialUpdate(false);
    }

    // Connection status is available to get with the function
    // mqtt.connected()
}

// Helper function for detecting button press (pin change).
bool checkButton(uint8_t _pin, bool *_oldState, bool _triggerState)
{
    // Return variable.
    bool _retValue = false;

    // Get the new button state.
    bool _newState = digitalRead(_pin);

    // Check the state.
    if ((_newState == _triggerState) && ((*_oldState) == !_triggerState))
        _retValue = true;

    // Update the state of the variable for the prev. state.
    (*_oldState) = _newState;

    // Return the state of the button press.
    return _retValue;
}