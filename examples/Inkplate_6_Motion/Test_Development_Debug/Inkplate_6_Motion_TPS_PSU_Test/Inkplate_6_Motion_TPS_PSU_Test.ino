#include <InkplateMotion.h>

Inkplate inkplate;

unsigned long tpsTpsTemperatureReadout = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Code started");
    
    inkplate.begin(INKPLATE_GL16);
    inkplate.epdPSU(1);
    inkplate.pmic.voltageAdjust(TPS651851_VADJ_VSET_14750);
}

void loop()
{
    if ((unsigned long)(millis() - tpsTpsTemperatureReadout) > 1000ULL)
    {
        tpsTpsTemperatureReadout = millis();
        int tpsTemperature = inkplate.pmic.getTemperature();
        Serial.print("TPS651851 Temperature: ");
        Serial.println(tpsTemperature, DEC);
    }

    char myBuffer[30];
    checkForSerial(&inkplate, &Serial, myBuffer, sizeof(myBuffer), 100ULL);
}

void checkForSerial(Inkplate *_inkplate, HardwareSerial *_serial, char *_buffer, int _bufferSize, uint32_t _timeout)
{
    if (getDataFromSerial(_serial, _buffer, _bufferSize, _timeout))
    {
        // Local variables to store the data from the serial.
        char _progString[10];
        int _vcomVoltage;
        int _sscanfResult;

        // Check for the keyword!
        _sscanfResult = sscanf(_buffer, "%dmV %s", &_vcomVoltage, _progString);

        if (_sscanfResult == 2)
        {
            // Convert string to lowercase.
            for (int i = 0; i < strlen(_progString); i++)
            {
                _progString[i] = tolower(_progString[i]);
            }

            // Check for the match.
            if ((strstr(_progString, "prog") != NULL) && (_vcomVoltage > -5000) && (_vcomVoltage <= 0))
            {
                if (_inkplate->pmic.programVCOM((float)(_vcomVoltage / 1000.0)))
                {
                    _serial->println("VCOM Programming OK!");
                }
                else
                {
                    _serial->println("VCOM Programing Failed!");
                }
            }
        }
    }
}

int getDataFromSerial(HardwareSerial *_serial, char *_buffer, int _bufferSize, uint32_t _timeout)
{
    // Return value, set it to false by default.
    bool _retValue = 0;

    // Check if there is something on serial.
    if (_serial->available())
    {
        // If there is something on UART, init the variables.
        int _index = 0;
        // Capture the timestamp for the timeout!
        unsigned long _timeoutCounter = millis();

        // Capture the data.
        while (((unsigned long)(millis() - _timeoutCounter) < _timeout))
        {
            // Check if there is new serail data available.
            if (_serial->available())
            {
                // If there is still space in the buffer, save the char!
                if (_index < (_bufferSize - 1))
                {
                    // Copy the char into the dedicated buffer.
                    _buffer[_index++] = _serial->read();

                    // Update the timeout counter vartiable.
                    _timeoutCounter = millis();
                }
                else
                {
                    // Buffer is full, drop the chars!
                    _serial->read();
                }
            }
        }

        // Add nul-terminating char.
        _buffer[_index] = '\0';

        // Return 1 fo success - data has been received.
        _retValue = 1;
    }

    // Retrun the result.
    return _retValue;
}
