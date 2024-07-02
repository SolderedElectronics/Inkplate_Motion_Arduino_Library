// Include Inkplate Motion Arduino Libary.
#include <InkplateMotion.h>

// Create Inkplate Motion Object.
Inkplate inkplate;

// Global buffer for RX AT commands from UART.
char buffer[4096];

// Timeout for UART RX data from last received char.
const unsigned long serialTimeout = 100ULL;

// Uncomment for HEX output.
//#define USE_HEX_OUTPUT

void setup()
{
    // Initialize Serail Communication at 115200 bauds, used for debugging.
    Serial.begin(115200);

    // Send hello message.
    Serial.println("Inkplate Motion Code Started!");

    // Power up the WiFi module.
    if (!WiFi.init())
    {
        Serial.println("WiFi initialization failed, code stopped!");

        // Stop the code.
        while (1);
    }
    
    // Otherwise WiFi Initialization was ok.
    Serial.println("WiFi Initialization OK! Send AT Commands");
}

void loop()
{
    // Passtrough everything from serial to the AT SPI driver.
    if(getAtFromSerial(&Serial, buffer, sizeof(buffer), serialTimeout))
    {
        // Just for test, echo it back!
        Serial.print("[ECHO]:");
        Serial.write(buffer, strlen((char*)buffer));
        Serial.flush();
        Serial.println();

        if (!WiFi.sendAtCommand(buffer))
        {
            Serial.println("[AT Command Send fail!]");
        }
    }

        // Check for the response.
        uint16_t len = 0;
        if (WiFi.getAtResponse(buffer, sizeof(buffer), 1000ULL, &len))
        {
            // Check is the response is not empty.
            if (len > 0)
            {
                // Print every byte received.
                for (int i = 0; i < len; i++)
                {
                    Serial.write(buffer[i]);
                }
                // Write a new line.
                Serial.println();
// If enabled, also print respose data in HEX.
#ifdef USE_HEX_OUTPUT
                // Print every byte received in HEX.
                for (int i = 0; i < len; i++)
                {
                    // Write HEX prefix.
                    Serial.print("0x");
                    // Write leading zero if needed.
                    if (buffer[i] <= 0x0F) Serial.print('0');
                    // Write byte in HEX.
                    Serial.print(buffer[i], HEX);
                    // Add a space between two HEX numbers.
                    Serial.print(", ");
                }
                // Write a new line.
                Serial.println();
#endif
            }
        }
}

bool getAtFromSerial(HardwareSerial *_serial, char *_buffer, uint16_t _bufferSize, unsigned long _timeout)
{
    // Return value.
    bool _retValue = false;

    // Check the pointers and buffer size. Return false if something is wrong.
    if ((_serial == NULL) || (_buffer == NULL) || (_bufferSize == 0)) return false;

    // Check if there is any data at the serial monitor.
    if (_serial->available())
    {
        // Ok, so there is something in the Serial RX buffer. Capture that!
        // Variable for buffer indexing.
        uint16_t _index = 0;

        // Variable for the timeout value.
        unsigned long _timeoutValue = millis();

        // Capture data until there is some data incomming or until timeout occurs.
        while ((unsigned long)(millis() - _timeoutValue) < _timeout)
        {
            // Check once again if there is something in the Serial RX buffer.
            if (_serial->available())
            {
                // Check if the buffer is full. If it is, start dropping RX data.
                if (_index < (_bufferSize - 1))
                {
                    // Save every char into array.
                    _buffer[_index++] = Serial.read();

                    // Update timeout variable.
                    _timeoutValue = millis();
                }
            }
        }

        // Add null-terminating char at the end.
        _buffer[_index] = '\0';

        // Check if anything is captured. Set _retValue to true. Otherwise, keep it to false.
        if (_index != 0) _retValue = true;
    }
    else
    {
        _retValue = false;
    }

    // Return success for received data otherwise false.
    return _retValue;
}