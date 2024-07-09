// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Change WiFi SSID and password here.
#define WIFI_SSID "Soldered-testingPurposes"
#define WIFI_PASS "Testing443"

// Create an Inkplate Motion Object.
Inkplate inkplate;

// Create the MQTT object.
// NOTE: Only one MQTT broker connection can be used even if multiple objects are created.
WiFiMQTT mqtt;

const char mqttTopicTx[] = "solderedTest/solderedInkplate";
const char mqttTopicRx[] = "solderedTest/solderedDasduino";

// Variables for storing button state to be able to detect state change.
bool publishBtnOldState = true;
bool disconnectBtnOldState = true;
bool unsubBtnOldState = true;

void setup()
{
    // Setup a Serial communication for debug at 115200 bauds.
    Serial.begin(115200);

    // Print an welcome message (to know if the Inkplate board and STM32 are alive).
    Serial.println("Inkplate Motion Code Started!");

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

    // Keep content on the screen for few more seconds.
    delay(1000);

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
        while (1);
    }
    inkplate.println("Connected!");
    inkplate.partialUpdate(true);

    // Try to subscribe to topic.
    inkplate.print("Subcribing to ");
    inkplate.print(mqttTopicRx);
    inkplate.print("topic...");
    inkplate.partialUpdate(true);
    if (!mqtt.subscribe((char*)mqttTopicRx))
    {
        inkplate.print("failed!");
        inkplate.partialUpdate(false);

        // Do not go any further if failed.
        while (1);
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
        if (mqtt.publish((char*)mqttTopicTx, message, 0, true))
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
        if (mqtt.unsubscribe((char*)mqttTopicRx))
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
        Serial.print("Connected? ");
        Serial.println(mqtt.connected()?"Yes":"No");
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
    if ((_newState == _triggerState) && ((*_oldState) == !_triggerState)) _retValue = true;

    // Update the state of the variable for the prev. state.
    (*_oldState) = _newState;

    // Return the state of the button press.
    return _retValue;
}