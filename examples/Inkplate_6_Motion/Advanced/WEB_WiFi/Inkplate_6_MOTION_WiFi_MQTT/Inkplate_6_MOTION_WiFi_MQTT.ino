/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_WiFi_MQTT.ino
 * @brief       This example will show you how to scan available WiFi networks and print them
 *
 *              To successfully run the sketch:
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
#define WIFI_SSID "Soldered"
#define WIFI_PASS "dasduino"

// Define the MQTT topic names
const char mqttTopicTx[] = "solderedTest/solderedInkplate";
const char mqttTopicRx[] = "solderedTest/solderedInkplate";

// Variables for storing button state to be able to detect state change.
bool publishBtnOldState = true;
bool disconnectBtnOldState = true;
bool unsubBtnOldState = true;

// Create an Inkplate Motion Object
Inkplate inkplate;

// Create the MQTT object.
// NOTE: Only one MQTT broker connection can be used even if multiple objects are created.
WiFiMQTT mqtt;

// Timer variable used for printing connection status every few seconds.
unsigned long timer1 = 0;

// Setup code, runs only once
void setup()
{
    Serial.begin(115200);

    // Initialize the Inkplate Motion Library in 1bit mode
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Use WAKE/USER1 pin for publishing MQTT data to the INPUT_PULLUP.
    pinMode(PC13, INPUT_PULLUP);
    // Use USER2 pin for the MQTT disconnect to the INPUT_PULLUP.
    pinMode(PG6, INPUT_PULLUP);
    // Use USER3 pin for the MQTT unsubscribe from the topic to the INPUT_PULLUP.
    pinMode(PA0, INPUT_PULLUP);

    // Set text printing option
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // Here' some technical information on how WiFi works on Inkplate 6 MOTION:
    // The onboard ESP32 is a co-processor, it's connected to the STM32 via SPI
    // It's running a firmware called ESP-AT: https://github.com/espressif/esp-at
    // This means, it will perform WiFi functions sent to it and communicate back to the STM32
    // This is all done seamlessly through the Inkplate Motion library

    // Initialize ESP32 WiFi
    if (!WiFi.init())
    {
        // If we're here, couldn't initialize WiFi, something is wrong!
        inkplate.println("ESP32-C3 initialization Failed! Code stopped.");
        inkplate.display();
        // Go to infinite loop after informing the user
        while (1)
        {
            delay(100);
        }
    }
    // Great, initialization was successful

    // Set mode to station
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA))
    {
        inkplate.println("STA mode failed!");
        inkplate.partialUpdate(true);
        // Somehow this didn't work, go to infinite loop!
        while (1)
        {
            delay(100);
        }
    }

    // Connect to the WiFi network.
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.print("...");
    inkplate.partialUpdate(true);
    // This is the function which connects to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait until connected
    while (!WiFi.connected())
    {
        // Print a dot for each second we're waiting
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    // Great, we're connected! Inform the user
    inkplate.println("connected!");
    inkplate.partialUpdate(true);
    delay(1000); // Wait a bit

    // Initialize MQTT library and use internal dynalically buffer of
    // 48 bytes for MQTT RX messages. Do the init. after WiFi connection.
    mqtt.begin(48);

    // Set the MQTT server data.
    mqtt.setServer("test.mosquitto.org", 1883);

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
    inkplate.println("Buttons: USER1 - Publish data, USER2 - Disconnect, USER3 - Unsubscribe\nMQTT traffic:");
    inkplate.display();
}


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
        Serial.println("sending data via mqtt");

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
        delay(5000);
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

    // Print connection status on serial every few seconds.
    if ((unsigned long)(millis() - timer1) > 10000ULL)
    {
        timer1 = millis();
    }
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