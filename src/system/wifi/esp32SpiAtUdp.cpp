/**
 **************************************************
 *
 * @file        esp32SpiAtUdp.cpp
 * @brief       Source file for the UDP communicaiton with the ESP32.
 *              This file is used with esp32SpiAt library.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include main header file for the UDP.
#include "esp32SpiAtUdp.h"

/**
 * @brief Construct for a new WiFiUDP object.
 *
 */
WiFiUDP::WiFiUDP()
{
    // Get the pointer address of the data buffer of the WiFi library.
    _dataBuffer = WiFi.getDataBuffer();

    // Heap, not a member array: examples create WiFiUDP on the stack.
    _rxBuffer = (char *)malloc(INKPLATE_ESP32_AT_CMD_BUFFER_SIZE);
}

/**
 * @brief   Destroy the WiFiUDP object and free the RX buffer.
 *
 */
WiFiUDP::~WiFiUDP()
{
    free(_rxBuffer);
}

/**
 * @brief   Initializer for the UDP.
 *
 * @param   uint16_t _localPort
 *          Local port number.
 * @return  bool
 *          true - Command execured succesfully.
 *          false - Command failed.
 */
bool WiFiUDP::begin(uint16_t _localPort)
{
    // Do not allow multiple connections.
    if (!WiFi.sendAtCommandWithResponse((char *)("AT+CIPMUX=0\r\n"), 200ULL, 4ULL, (char *)esp32AtCmdResponseOK,
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // Store local port.
    _localUdpPort = _localPort;

    // Reset number of available bytes (in case of the re-usage of the object).
    _availableData = 0;

    // Return true if you got here.
    return true;
}

/**
 * @brief   Sets the host by name or by IP address.
 *
 * @param   const char * _host
 *          Host name.
 * @param   uint16_t _hostPort
 *          Host port for UDP.
 * @return  bool
 *          true - Command executed successfully.
 *          false - Host name and host port set failed.
 */
bool WiFiUDP::setHost(const char *_host, uint16_t _hostPort)
{
    // Create AT commands with parameters.
    sprintf(_dataBuffer, "AT+CIPSTART=\"UDP\",\"%s\",%d,%d,2\r\n", _host, _hostPort, _localUdpPort);

    // Send command and check response. It should respond with CONNECT. Otherwise return false.
    if (!WiFi.sendAtCommandWithResponse(_dataBuffer, _connectionTimeoutValue, 4ULL, (char *)"CONNECT",
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // If you got here, everything went ok, return true.
    return true;
}

/**
 * @brief   Initializer for packet transfer.
 *
 * @return  bool
 *          true - UDP packet initialization was successfull.
 *          false - UDP packet initialization has failed.
 */
bool WiFiUDP::beginPacket()
{
    // Reset number of available bytes (in case of the re-usage of the object).
    _availableData = 0;

    // Enable the message filter for the response.
    // Remove "Recv XY bytes\r\n"
    if (!WiFi.messageFilter(true, "^Recv [0-9]* bytes", "\r\n$"))
        return false;

    // Remove "busy p..." — ESP32 sends this after receiving payload to indicate
    // it is processing the UDP send. Without this filter it lands in Phase 3 and
    // is mistaken for the echo reply, causing constant garbage RX values.
    if (!WiFi.messageFilter(true, "^busy p...", "\r\n$"))
        return false;

    // Remove "+IPD,XY:" header from incoming data notifications.
    if (!WiFi.messageFilter(true, "^+IPD,[0-9]*:", "\r\n$"))
        return false;

    // If you got here, everything went ok, return true.
    return true;
}

// Binary-safe substring search (received data may contain 0x00). Returns the first match or NULL.
static char *memFind(char *_buf, uint16_t _len, const char *_needle)
{
    size_t _n = strlen(_needle);
    for (uint16_t i = 0; i + _n <= _len; i++)
        if (memcmp(_buf + i, _needle, _n) == 0)
            return _buf + i;
    return NULL;
}

static bool memContains(char *_buf, uint16_t _len, const char *_needle)
{
    return memFind(_buf, _len, _needle) != NULL;
}

/**
 * @brief   Send data to the desired web server to it's destination port.
 *
 * @param   uint8_t *_packet
 *          Pointer to the packet that will be sent to the UDP.
 * @param   uint16_t _len
 *          Number of bytes to write (size of the packet).
 *
 * @return  bool
 *          true - Packet write was successfull.
 *          false - packet write failed.
 */
bool WiFiUDP::write(uint8_t *_packet, uint16_t _len)
{
    // Phase 1: Send AT+CIPSEND=N, look for OK anywhere in accumulated response.
    // Use local buffer — sendAtCommand calls flushModemReadReq() which overwrites
    // _dataBuffer. Passing _dataBuffer directly corrupts the AT command on flush.
    // Time-based loop: flood collision on dataSendRequest's handshake wait causes
    // READABLE instead of WRITEABLE. Drain flood and retry until 2s total.
    // Phase 1 failures are safe (ESP32 not yet in data-mode) so returning false here
    // does not corrupt state.
    char _cmd[24];
    sprintf(_cmd, "AT+CIPSEND=%d\r\n", _len);
    bool _p1sent = false;
    unsigned long _p1start = millis();
    while (!_p1sent && (millis() - _p1start) < 2000ULL)
    {
        _p1sent = WiFi.sendAtCommand(_cmd);
        if (!_p1sent)
            WiFi.getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 20ULL, NULL);
    }
    if (!_p1sent)
        return false;

    // Wait for the "OK" + ">" prompt packet by packet and stop as soon as it is seen, instead of waiting for a
    // 100 ms quiet gap on every write. Search binary-safe: inbound UDP data arriving meanwhile can contain 0x00,
    // which made strstr() miss the OK and fail the write.
    uint16_t _p1Len = 0;
    bool _prompt = false;
    unsigned long _p1wait = millis();
    while (!_prompt && (millis() - _p1wait) < 1000ULL)
    {
        uint16_t _len = 0;
        if (!WiFi.getSimpleAtResponse(_dataBuffer + _p1Len, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE - _p1Len, 100ULL, &_len))
            continue;
        _p1Len += _len;
        if (memContains(_dataBuffer, _p1Len, "ERROR"))
            return false;
        _prompt = memContains(_dataBuffer, _p1Len, "OK\r\n") && memContains(_dataBuffer, _p1Len, ">");
    }
    if (!_prompt)
        return false;

    // Phase 2: Send payload. Time-based loop — must succeed to exit data-mode.
    // If Phase 1 got OK, ESP32 is in data-mode waiting for _len bytes. Returning
    // false without sending permanently corrupts the AT state machine.
    // _packet is not _dataBuffer so flush inside sendAtCommand cannot corrupt it.
    bool _p2sent = false;
    unsigned long _p2start = millis();
    while (!_p2sent && (millis() - _p2start) < 2000ULL)
    {
        _p2sent = WiFi.sendAtCommand((char *)_packet, _len);
        if (!_p2sent)
            WiFi.getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 20ULL, NULL);
    }
    if (!_p2sent)
        return false;

    // Phase 3: wait for the ESP32's send confirmation. write() reports whether the packet was sent; replies are
    // read with available()/read(). Inbound data arriving before SEND OK (e.g. a fast reply) is kept for read().
    unsigned long _p3start = millis();
    while ((millis() - _p3start) < 1000ULL)
    {
        uint16_t _rxLen = 0;
        if (!WiFi.getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 100ULL, &_rxLen))
            continue;
        if (memContains(_dataBuffer, _rxLen, "SEND OK\r\n"))
        {
            // The confirmation can share an SPI packet with inbound data - keep everything except the token.
            char *_tok = memFind(_dataBuffer, _rxLen, "\r\nSEND OK\r\n");
            if (_tok != NULL)
            {
                uint16_t _tokLen = strlen("\r\nSEND OK\r\n");
                uint16_t _before = _tok - _dataBuffer;
                keepRxData(_dataBuffer, _before);
                keepRxData(_tok + _tokLen, _rxLen - _before - _tokLen);
            }
            return true;
        }
        if (memContains(_dataBuffer, _rxLen, "SEND FAIL") || memContains(_dataBuffer, _rxLen, "\r\nERROR\r\n"))
            return false;
        keepRxData(_dataBuffer, _rxLen);
    }
    return false;
}

/**
 * @brief   Append received data to the RX buffer so available()/read() return it.
 *
 * @param   char *_data
 *          Received data.
 * @param   uint16_t _len
 *          Number of received bytes.
 */
void WiFiUDP::keepRxData(char *_data, uint16_t _len)
{
    if ((_rxBuffer == NULL) || (_len == 0))
        return;

    // Move unread data to the start, then append if it fits (otherwise the new data is dropped).
    if ((_availableData != 0) && (_currentPosition != _rxBuffer))
        memmove(_rxBuffer, _currentPosition, _availableData);
    _currentPosition = _rxBuffer;
    if ((uint32_t)_availableData + _len <= INKPLATE_ESP32_AT_CMD_BUFFER_SIZE)
    {
        memcpy(_rxBuffer + _availableData, _data, _len);
        _availableData += _len;
    }
}

// Returns how many bytes are available for read.

/**
 * @brief   Returns how many bytes are available for read.
 *
 * @param   bool _blocking
 *          true - use blocking (wait for new packets).
 *          false - Do not wait for new packets, just check.
 * @return  uint16_t
 *          Number of available bytes to read from the buffer.
 */
uint16_t WiFiUDP::available(bool _blocking)
{
    if ((_availableData == 0) && (_rxBuffer != NULL))
    {
        uint16_t _timeoutValue = _blocking ? 2500ULL : 20UL;
        uint16_t _len = 0;

        if (WiFi.getSimpleAtResponse(_rxBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, _timeoutValue, &_len))
        {
            // A late send confirmation (write() timed out before it came) is not received data.
            if (!((_len <= 13) && memContains(_rxBuffer, _len, "SEND OK\r\n")))
            {
                _availableData += _len;
                _currentPosition = _rxBuffer;
            }
        }
    }

    return _availableData;
}

// read bytes from the RX buffer.

/**
 * @brief   Read bytes from the RX buffer.
 *
 * @param   uint8_t *_data
 *          Pointer to buffer where to store data.
 * @param   uint16_t _len
 *          How many bytes to read from the buffer.
 * @return  int
 *          How many bytes are actually read from the buffer (in the case available data < _len).
 */
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

/**
 * @brief   End UDP connection. Also remove all filters for data transfer.
 *
 * @return  bool
 *          true - ESP was successfully terminated connection.
 *          false - ESP failed to terminate connection or to remove message filters.
 */
bool WiFiUDP::end()
{
    _availableData = 0;
    _currentPosition = NULL;

    // Remove the filters beginPacket() added. Without this, every begin()/beginPacket()/end()
    // cycle re-adds duplicates and never frees them, eventually exhausting the ESP32's filter
    // table and making every further AT+SYSMSGFILTERCFG add permanently fail.
    WiFi.messageFilter(false, "^Recv [0-9]* bytes", "\r\n$");
    WiFi.messageFilter(false, "^busy p...", "\r\n$");
    WiFi.messageFilter(false, "^+IPD,[0-9]*:", "\r\n$");

    // Disconnect from the server.
    if (!WiFi.sendAtCommandWithResponse("AT+CIPCLOSE\r\n", 2000ULL, 4ULL, (char *)("CLOSED"),
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // At the end, return true.
    return true;
}

// Sets connection timeout in milliseconds. Must be called before begin!

/**
 * @brief   Sets connection timeout in milliseconds.
 *
 * @param   uint16_t _connectionTimeout
 *          Connection timeout in milliseconds.
 *
 * @note    Must be called before begin!
 */
void WiFiUDP::setConnectionTimeout(uint16_t _connectionTimeout)
{
    _connectionTimeoutValue = _connectionTimeout;
}