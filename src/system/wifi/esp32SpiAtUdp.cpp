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

    // Passive receive mode: the ESP32 buffers received data and only announces it ("+IPD,<len>"); it is fetched
    // with AT+CIPRECVDATA. In active mode the ESP32 can put AT replies in the middle of received data, so the two
    // cannot be told apart reliably.
    if (!WiFi.sendAtCommandWithResponse((char *)("AT+CIPRECVMODE=1\r\n"), 200ULL, 4ULL, (char *)esp32AtCmdResponseOK,
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // Store local port.
    _localUdpPort = _localPort;

    // Reset received data (in case of the re-usage of the object).
    _rxLen = 0;
    _rdOff = 0;
    WiFi.clearRxPending();

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
    // Enable the message filter for the response.
    // Remove "Recv XY bytes\r\n"
    if (!WiFi.messageFilter(true, "^Recv [0-9]* bytes", "\r\n$"))
        return false;

    // Remove "busy p..." — ESP32 sends this after receiving payload to indicate
    // it is processing the UDP send. Without this filter it lands in Phase 3 and
    // is mistaken for the echo reply, causing constant garbage RX values.
    if (!WiFi.messageFilter(true, "^busy p...", "\r\n$"))
        return false;

    // "+IPD,<len>" is deliberately not filtered: it is the passive-mode notification that data is waiting.

    // If you got here, everything went ok, return true.
    return true;
}

// Binary-safe substring search (buffer contents are not guaranteed to be null-terminated).
static bool memContains(const char *_buf, uint16_t _len, const char *_needle)
{
    size_t _n = strlen(_needle);
    for (uint16_t i = 0; i + _n <= _len; i++)
        if (memcmp(_buf + i, _needle, _n) == 0)
            return true;
    return false;
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
    // read with available()/read(). "SEND OK" can be split across reads - collect it.
    uint16_t _p3Len = 0;
    unsigned long _p3start = millis();
    while ((millis() - _p3start) < 1000ULL)
    {
        uint16_t _rxLen = 0;
        if (!WiFi.getSimpleAtResponse(_dataBuffer + _p3Len, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE - _p3Len, 100ULL,
                                      &_rxLen))
            continue;
        _p3Len += _rxLen;
        if (memContains(_dataBuffer, _p3Len, "SEND OK\r\n"))
            return true;
        if (memContains(_dataBuffer, _p3Len, "SEND FAIL") || memContains(_dataBuffer, _p3Len, "ERROR\r\n"))
            return false;
    }
    return false;
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
    if (_rxBuffer == NULL)
        return 0;

    // Unread data from the last fetch first.
    if (_rdOff < _rxLen)
        return _rxLen - _rdOff;

    // Wait for the ESP32's "+IPD" notification (any WiFi read picks it up), then fetch everything it buffered.
    unsigned long _timeoutValue = _blocking ? 2500ULL : 20UL;
    unsigned long _start = millis();
    while (true)
    {
        // Safety net: the ESP32 notifies only when its buffer goes from empty to non-empty, so a notification
        // that got lost would stop receiving for good. Ask for the buffered length now and then instead.
        bool _fetch = WiFi.rxPending();
        if (!_fetch && ((unsigned long)(millis() - _lastRxCheck) >= 100ULL))
        {
            _lastRxCheck = millis();
            _fetch = (WiFi.recvLen() > 0);
        }

        if (_fetch)
        {
            int16_t _n = WiFi.recvData(_rxBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE / 2);
            if (_n > 0)
            {
                _rxLen = _n;
                _rdOff = 0;
                return _rxLen;
            }
        }
        unsigned long _elapsed = millis() - _start;
        if (_elapsed >= _timeoutValue)
            return 0;
        WiFi.pollRx(_timeoutValue - _elapsed);
    }
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
    if ((_rxBuffer == NULL) || (_rdOff >= _rxLen))
        return 0;

    if (_len > (_rxLen - _rdOff))
        _len = _rxLen - _rdOff;
    memcpy(_data, _rxBuffer + _rdOff, _len);
    _rdOff += _len;

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
    _rxLen = 0;
    _rdOff = 0;

    // Remove the filters beginPacket() added. Without this, every begin()/beginPacket()/end()
    // cycle re-adds duplicates and never frees them, eventually exhausting the ESP32's filter
    // table and making every further AT+SYSMSGFILTERCFG add permanently fail.
    WiFi.messageFilter(false, "^Recv [0-9]* bytes", "\r\n$");
    WiFi.messageFilter(false, "^busy p...", "\r\n$");

    // Disconnect from the server.
    bool _closed = WiFi.sendAtCommandWithResponse("AT+CIPCLOSE\r\n", 2000ULL, 4ULL, (char *)("CLOSED"),
                                                  INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true);

    // Back to the default (active) receive mode that begin() changed.
    WiFi.sendAtCommandWithResponse((char *)("AT+CIPRECVMODE=0\r\n"), 200ULL, 4ULL, (char *)esp32AtCmdResponseOK,
                                   INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true);
    WiFi.clearRxPending();

    return _closed;
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