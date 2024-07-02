// Include main header file for the UDP.
#include "esp32SpiAtUdp.h"

WiFiUDP::WiFiUDP()
{
    // Get the pointer address of the data buffer of the WiFi library.
    _dataBuffer = WiFi.getDataBuffer();
}

// Initializer for the UDP.
bool WiFiUDP::begin(uint16_t _localPort)
{
    // Do not allow multiple connections.
    if (!WiFi.sendAtCommandWithResponse((char*)("AT+CIPMUX=0\r\n"), 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true)) return false;

    // Store local port.
    _localUdpPort = _localPort;

    // Reset number of available bytes (in case of the re-usage of the object).
    _availableData = 0;

    // Return true if you got here.
    return true;
}

// Sets the host by name or by IP address.
bool WiFiUDP::setHost(const char *_host, uint16_t _hostPort)
{
    // Create AT commands with parameters.
    sprintf(_dataBuffer, "AT+CIPSTART=\"UDP\",\"%s\",%d,%d,2\r\n", _host, _hostPort, _localUdpPort);

    // Send command and check response. It should respond with CONNECT. Otherwise return false.
    if (!WiFi.sendAtCommandWithResponse(_dataBuffer, _connectionTimeoutValue, 4ULL, (char*)"CONNECT", INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true)) return false;

    // If you got here, everything went ok, return true.
    return true;
}

// Initializer for packet transfer.
bool WiFiUDP::beginPacket()
{
    // Reset number of available bytes (in case of the re-usage of the object).
    _availableData = 0;

    // Enable the message filter for the response.
    // Remove "Recv XY bytes\r\n"
    if (!WiFi.messageFilter(true, "^Recv [0-9]* bytes", "\r\n$")) return false;

    // Remove "SEND OK".
    if (!WiFi.messageFilter(true, NULL, "\r\nSEND OK\r\n")) return false;

    // Remove "+IPD,XY".
    if (!WiFi.messageFilter(true, "^+IPD,[0-9]*:", "\r\n$")) return false;

    // If you got here, everything went ok, return true.
    return true;
}

// Send data to the desired web server to it's destination port.
bool WiFiUDP::write(uint8_t *_packet, uint16_t _len)
{
    // Send the UDP packet! If failed, return false.
    sprintf(_dataBuffer, "AT+CIPSEND=%d\r\n", _len);
    if (!WiFi.sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, "\r\nOK\r\n\r\n>", INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, (char*)_packet, _len, 20000ULL, NULL, &_availableData)) return false;

    // Set the current positon pointer at the start of the RX buffer.
    _currentPosition = _dataBuffer;

    // If you got here and you got some data, everything went ok, return ture.
    return _availableData != 0?true:false;
}

// Returns how many bytes are available for read.
uint16_t WiFiUDP::available(bool _blocking)
{
    // Only get new data if the current buffer is empty.
    if (_availableData == 0)
    {
        // Calculate the timeout value for new data. If blocking method is enabled,
        // use longer timeout value. Otherwise, use shorter timeout value (but in this case user
        // must create some kind of mechanism to know when all data has been received).
        uint16_t _timeoutValue = _blocking ? 2500ULL : 20UL;

        // Set variable for data chunk size to zero.
        uint16_t _len = 0;

        // Try to get new data. If new data is available, update the size and current pointer for the data.
        if (WiFi.getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, _timeoutValue, &_len))
        {
            _availableData += _len;
            _currentPosition = _dataBuffer;
        }
    }

    // Return the current buffer size.
    return _availableData;
}

// read bytes from the RX buffer.
int WiFiUDP::read(uint8_t *_data, uint16_t _len)
{
    // If there is no new data, return 0.
    if (_currentPosition == NULL)
        return 0;

    // Check if the buffer length is larger than received data.
    // If so, set the length to the received data length.
    if (_len > _availableData)
        _len = _availableData;

    // Copy data from internal buffer to the provided oone.
    memcpy(_data, _currentPosition, _len);

    // Update the variables for offset and data length.
    _availableData -= _len;
    _currentPosition += _len;

    // Return the actual length.
    return _len;
}

// End UDP connection. Also remove all filters for data transfer.
bool WiFiUDP::end()
{
    _availableData = 0;
    _currentPosition = NULL;

    // Re-enable filters 
    WiFi.messageFilter(true, "^Recv [0-9]* bytes", "\r\n$");
    WiFi.messageFilter(true, NULL, "\r\nSEND OK\r\n");
    WiFi.messageFilter(true, "^+IPD,[0-9]*:", "\r\n$");

    // Disconnect from the server.
    if (!WiFi.sendAtCommandWithResponse("AT+CIPCLOSE\r\n", 2000ULL, 4ULL, (char*)("CLOSED"), INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true)) return false;

    // At the end, return true.
    return true;
}

// Sets connection timeout in milliseconds. Must be called before begin!
void WiFiUDP::setConnectionTimeout(uint16_t _connectionTimeout)
{
    _connectionTimeoutValue = _connectionTimeout;
}