/**
 **************************************************
 *
 * @file        esp32SpiAtMqtt.cpp
 * @brief       Source file for the MQTT communication with the ESP32.
 *              This file is used with esp32SpiAt library.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Innclude main header file.
#include "esp32SpiAtMqtt.h"

/**
 * @brief   Construct a new WiFiMQTT:: object. It gets the main At Command Buffer address.
 *
 */
WiFiMQTT::WiFiMQTT()
{
    // Get the pointer of the data buffer of the ESP32 SPI WiFi library.
    _atCommandBuffer = WiFi.getDataBuffer();
}

/**
 * @brief   Destructor for the WiFiMQTT object. It will free allocated memory on the
 *          object destruct.
 *
 */
WiFiMQTT::~WiFiMQTT()
{
    // Free up the allocated memory (but only if is allocated by malloc, not user defined buffer!).
    if (_allocated)
    {
        // Free up allocated memory.
        free(_rxDataBuffer);

        // Set the flag to false.
        _allocated = false;
    }
}

/**
 * @brief   Initializer for the WiFiMQTT library. It will allocate memory for the incomming
 *          MQTT messages.
 *
 * @param   uint16_t _rxBufferSize
 *          Size of the allocated buffer (in bytes).
 * @return  bool
 *          true - Memory allocation is successful.
 *          false - Memory allocation failed.
 */
bool WiFiMQTT::begin(uint16_t _rxBufferSize)
{
    // Allocate the memory for the RX buffer. If not specified, buffer will be 1024 bytes.
    _rxDataBuffer = (char *)malloc(_rxBufferSize * sizeof(uint8_t));

    // Check for the success memory allocation.
    if (_rxDataBuffer != NULL)
    {
        // Set the flag.
        _allocated = true;

        // Copy buffer size locally in the class.
        _maxRxBufferSize = _rxBufferSize;

        // Return true for success.
        return true;
    }

    // Allocation failed? Return false.
    return false;
}

/**
 * @brief   Initializer for the WiFiMQTT library. Use user-defined buffer for incomming MQTT messages.
 *
 * @param   uint8_t *_rxBuffer
 *          Pointer to the user defined buffer for incomming MQTT messages.
 * @param   uint16_t _rxBufferSize
 *          Buffer size in bytes.
 * @return  bool
 *          true - Buffer will be used as buffer for the incomming MQTT messages.
 *          false - Buffer cannot be used (maybe thev address of the buffer is nul?).
 */
bool WiFiMQTT::begin(uint8_t *_rxBuffer, uint16_t _rxBufferSize)
{
    // Check the pointer (watch out for null pointer!).
    if (_rxBuffer == NULL)
        return false;

    // Save adday address locally.
    _rxDataBuffer = (char *)_rxBuffer;

    // Copy buffer size locally in the class.
    _maxRxBufferSize = _rxBufferSize;

    // Set the flag.
    _allocated = false;

    // Return true for success.
    return true;
}

/**
 * @brief   Method set the MQTT server domain or IP address and port.
 *
 * @param   char *_mqttServer
 *          MQTT Server/Broker domain or IP address.
 * @param   uint16_t _mqttPort
 *          MQTT Server/Broker port (usually 1883).
 * @return  bool
 *          true - MQTT server/broker is set correctly
 *          false - MQTT server set is faild (check _mqttServer array address, maybe is NULL?).
 * @note    This does not instantlly set these data to the ESP32, instead it stores them locally, so it's
 *          advisable to not use locally variables for MQTT server.
 */
bool WiFiMQTT::setServer(char *_mqttServer, uint16_t _mqttPort)
{
    // Just save the data internally. Data will be used in WiFiMQTT:connect().
    // But do not forget check null pointer!
    if (_mqttServer == NULL)
        return false;

    // Copy parameters locally.
    _serverPtr = _mqttServer;
    _port = _mqttPort;

    // Return true.
    return true;
}

/**
 * @brief   Connect to the MQTT Server with MQTT ClientID, Username and password.
 *
 * @param   char *_clientId
 *          ClientID used in MQTT. If not used, use NULL pointer.
 * @param   char *_userName
 *          Username for auth used in MQTT. If not used, use NULL pointer.
 * @param   char *_password
 *          Password for auth used in MQTT. If not used, use NULL pointer.
 * @return  bool
 *          true - Connection to the MQTT broker/server was successfull.
 *          false - Connection failed.
 */
bool WiFiMQTT::connect(char *_clientId, char *_userName, char *_password)
{
    // First set MQTT user config. Keep ClientIC, username and password blank sice these will be updated by using
    // dedicated AT Commands AT+MQTTLONGCLIENTID, AT+MQTTLONGUSERNAME and AT+MQTTLONGPASSWORD.
    if (!WiFi.sendAtCommandWithResponse((char *)"AT+MQTTUSERCFG=0,1,\"\",\"\",\"\",0,0,\"\"\r\n", 200ULL, 4ULL,
                                        (char *)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // First set up the client ID if needed.
    if (_clientId != NULL)
    {
        sprintf(_atCommandBuffer, "AT+MQTTLONGUSERNAME=0,%d\r\n", strlen(_clientId));
        if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 200ULL, 4ULL, (char *)"\r\nOK\r\n\r\n>",
                                            INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, _clientId,
                                            strlen(_clientId), 20ULL, (char *)esp32AtCmdResponseOK))
            return false;
    }

    // Now set up the username in nedeed.
    if (_userName != NULL)
    {
        sprintf(_atCommandBuffer, "AT+MQTTLONGUSERNAME=0,%d\r\n", strlen(_userName));
        if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 200ULL, 4ULL, (char *)"\r\nOK\r\n\r\n>",
                                            INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, _userName,
                                            strlen(_userName), 20ULL, (char *)esp32AtCmdResponseOK))
            return false;
    }

    // Now set up the password in needed.
    if (_password != NULL)
    {
        sprintf(_atCommandBuffer, "AT+MQTTLONGPASSWORD=0,%d\r\n", strlen(_password));
        if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 200ULL, 4ULL, (char *)"\r\nOK\r\n\r\n>",
                                            INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, _password,
                                            strlen(_password), 20ULL, (char *)esp32AtCmdResponseOK))
            return false;
    }

    // At the last thing, try to connect to the client.
    sprintf(_atCommandBuffer, "AT+MQTTCONN=0,\"%s\",%d,%d\r\n", _serverPtr, _port, _reconnect);
    // Send AT command and wait  for the response.
    if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 60000ULL, 4ULL, (char *)"+MQTTCONNECTED",
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_ANY, true))
        return false;

    // If everything went ok, return true.
    return true;
}

/**
 * @brief   Subscribe to the topic on MQTT.
 *
 * @param   const char *_topic
 *          Name of the topic to subscribe on.
 * @return  bool
 *          true  - Subscribe on the topic was successful.
 *          false - Subscribe on the topic failed.
 */
bool WiFiMQTT::subscribe(const char *_topic)
{
    // Return value.
    bool _retValue = true;

    // Make a command.
    sprintf(_atCommandBuffer, "AT+MQTTSUB=0,\"%s\",%d\r\n", _topic, _QoSParameter);
    // Send a AT Command and get the response. If connected, it should return "+MQTTSUBRECV" or just "OK".
    // If already subscribed response will be "ALREADY SUBSCRIBE". This is also a vaild response.
    if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 20000ULL, 4ULL,
                                        (char *)"+MQTTSUBRECV:", INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
    {
        // False means that we expected "+MQTTSUBRECV" but got OK or "ALREADY SUBSCRIBE". If non of there were received,
        // return false!
        if ((strstr(_atCommandBuffer, "ALREADY SUBSCRIBE") != NULL) || (strstr(_atCommandBuffer, "\r\nOK\r\n") != NULL))
            _retValue = true;
    }

    // Try to parse the data sent by the MQTT while subscribe.
    parseMQTTData("+MQTTSUBRECV:");

    // Return _retValue (true - subscribed to the topic).
    return _retValue;
}

/**
 * @brief   Get the topic name from the last received data.
 *
 * @return  char*
 *          Pointer to the array that holds last received topic name.
 *
 * @note    Max length is 254 bytes. In case of longer topic names, only first 254 bytes will be used.
 */
char *WiFiMQTT::topic()
{
    // Just return last received topic.
    return _lastRxTopic;
}

/**
 * @brief   Disconnect from the MQTT server/broker. Only one connection to the server
 *          is possible, no multiple connections are possible, even when using multiple objects.
 *
 * @return  bool
 *          true - Disconnect from server/broker was successful.
 *          false - Disconnect failed.
 */
bool WiFiMQTT::disconnect()
{
    // Send AT Command (AT+MQTTCLEAN) for disconnect from the MQTT broker.
    if (!WiFi.sendAtCommandWithResponse("AT+MQTTCLEAN=0\r\n", 20000ULL, 4ULL, (char *)esp32AtCmdResponseOK,
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // If OK is found, disconnect was successful, return true.
    return true;
}

/**
 * @brief   Unsubscribe from the topic (stop receiving messages from that topic).
 *
 * @param   char *_topic
 *          Topic name to unsubscribe from.
 * @return  bool
 *          true - Unsubscribe from the topic was successful.
 *          false - Unsuibscribe failed.
 *
 * @note    Length of the topic name should not exceed 128 bytes.
 */
bool WiFiMQTT::unsubscribe(char *_topic)
{
    // Use AT+MQTTUNSUB=<LinkID>,<"topic">.
    sprintf(_atCommandBuffer, "AT+MQTTUNSUB=0,\"%s\"\r\n", _topic);

    // Send AT command and wait for response. It should return OK.
    if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 20000ULL, 4ULL, (char *)esp32AtCmdResponseOK,
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true))
        return false;

    // If OK is found, return true.
    return true;
}

/**
 * @brief   Publish data to the specific topic.
 *
 * @param   char *_topic
 *          Selected topic where to publish data.
 * @param   char *_payload
 *          What needs to be sent to the topic.
 * @param   uint16_t _len
 *          Lenght of the data (payload). It the payload is string (nul-terminated), keep the lenght 0.
 * @param   bool _retain
 *          Message retain flag. True - Keep message.
 * @return  bool
 *          true - Publish was successful.
 *          false - Publish failed.
 * @note    Timeout for the publish is 2000ms.
 */
bool WiFiMQTT::publish(char *_topic, char *_payload, uint16_t _len, bool _retain)
{
    // First get the size of the payload. If the _len is 0, that means _payload is string.
    if (_len == 0)
        _len = strlen(_payload);

    // Use AT+MQTTPUBRAW=<LinkID>,<"topic">,<length>,<qos>,<retain> command.
    sprintf(_atCommandBuffer, "AT+MQTTPUBRAW=0,\"%s\",%d,%d,%d\r\n", _topic, _len, _QoSParameter, _retain & 1);

    // Send AT command and data.
    if (!WiFi.sendAtCommandWithResponse(_atCommandBuffer, 200ULL, 4ULL, (char *)"\r\nOK\r\n\r\n>",
                                        INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, _payload, _len, 20000ULL,
                                        "+MQTTPUB:OK\r\n"))
        return false;

    // If you got here, return true.
    return true;
}

/**
 * @brief   Check if the connection with MQTT server established.
 *
 * @return  bool
 *          true - Connected to the MQTT broker/server.
 *          false - Connection with MQTT is not established or ESP32 have disconnected from the MQTT.
 */
bool WiFiMQTT::connected()
{
    // Variables needed for parsing response.
    int _linkId = 0;
    int _state = 0;

    // Check the connection status by sending AT+MQTTCONN? Query.
    if (!WiFi.sendAtCommand("AT+MQTTCONN?\r\n"))
        return false;

    if (!WiFi.sendAtCommandWithResponse("AT+MQTTCONN?\r\n", 200ULL, 10ULL,
                                        (char *)"+MQTTCONN:", INKPLATE_ESP32_AT_EXPECTED_RESPONSE_ANY, true))
        return false;

    // Sonce the response can be anywhere in the message, try to find the start of the response.
    char *_responseStart = strstr(_atCommandBuffer, "+MQTTCONN:");
    if (_responseStart == NULL)
        return false;

    // Parse the response. The only needed parameter is <state> (second parameter in response).
    if (sscanf(_responseStart, "+MQTTCONN:%d,%d", &_linkId, &_state) != 2)
        return false;

    // Only 4, 5 and 6 means that is connedted to the MQTT broker.
    // 0 = MQTT uninitialized.
    // 1 = already set AT+MQTTUSERCFG.
    // 2 = already set AT+MQTTCONNCFG.
    // 3 = connection disconnected.
    // 4 = connection established.
    // 5 = connected, but did not subscribe to any topic.
    // 6 = connected, and subscribed to MQTT topics.
    return ((_state >= 3) ? true : false);
}

/**
 * @brief   Enable or disable automatic reconnect (automatic reconnect can use more resouces).
 *
 * @param   uint8_t _reconnectMode
 *          0 - Do not reconnect automatically. Reconnect must be done manually.
 *          1 - Reconnect automatically.
 *
 * @note    In the case of connection drop, before manual reconnect, call disconnect().
 *          Otherwise connect will fail!
 */
void WiFiMQTT::reconnect(uint8_t _reconnectMode)
{
    // Store internally, it fill be used in connect().
    _reconnect = _reconnectMode;
}

/**
 * @brief   Set QoS parameter. It can be 0, 1, or 2.
 *
 * @param   int _qos
 *          Quality of Service parameter, used in publish().
 *          0 - At most once
 *          1 - At least once
 *          2 - Exactly once
 *
 * @note    This parameter is set internally and used while publish()!
 */
void WiFiMQTT::setQoS(int _qos)
{
    // Set the internal variable for QoS. It will be set in the subscribe().
    _QoSParameter = _qos;
}

/**
 * @brief   Read incomming data from the subscribed topics.
 *
 * @param   uint8_t *_buffer
 *          Pointer to the buffer where the incomming message will be stored.
 * @param   uint16_t _len
 *          How many data will be stored in the user proveded buffer.
 * @return  uint16_t
 *          How many bytes are actually strored if the requested number of bytes exceed number of available bytes.
 */
uint16_t WiFiMQTT::read(uint8_t *_buffer, uint16_t _len)
{
    // If there is no new data, return 0.
    if (_currentPosition == NULL)
        return 0;

    // Check if the buffer length is larger than received data.
    // If so, set the length to the received data length.
    if (_len > _dataAvailable)
        _len = _dataAvailable;

    // Copy data from internal buffer to the provided oone.
    memcpy(_buffer, _currentPosition, _len);

    // Update the variables for offset and data length.
    _dataAvailable -= _len;
    _currentPosition += _len;

    // Return the actual length.
    return _len;
}

/**
 * @brief   Read one byte from the MQTT incomming message.
 *
 * @return  char
 *          One byte from the buffer. It will return 0 if no data available.
 */
char WiFiMQTT::read()
{
    // Return value - currently oldest byte in the array.
    // Set default value to zero if no data available.
    char _retValue = 0;

    // Check if there is any data available.
    if (_dataAvailable)
    {
        // Get the byte and increment pointer position.
        _retValue = *(_currentPosition++);

        // Update number of available bytes.
        _dataAvailable--;
    }

    // Return byte.
    return _retValue;
}

/**
 * @brief   Returns number of received bytes from MQTT.
 *
 * @return  uint16_t
 *          Number of received bytes available for read.
 */
uint16_t WiFiMQTT::available()
{
    // Return the bytes available.
    return _dataAvailable;
}

/**
 * @brief   Loop method that checks for new incomming data. Needs to be checked periodically.
 *
 * @note    It's recommended to not to use any other protocol while using MQTT since it can leda to loss of data.
 */
void WiFiMQTT::loop()
{
    // Get new data only if buffer is empty.
    if (!_dataAvailable)
    {
        uint16_t _len = 0;
        if (WiFi.getSimpleAtResponse(_atCommandBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 1ULL, &_len))
        {
            // Try to parse the data. Command is sent by the ESP32 as soon as topic is changed.
            // Response: +MQTTSUBRECV:0,"solderedTest/soldered",10,test_36570

            // Add nul-terminating char at the end.
            _atCommandBuffer[_len] = '\0';

            // Try to parse MQTT data.
            parseMQTTData("+MQTTSUBRECV:");
        }
    }
}

void WiFiMQTT::parseMQTTData(char *_cmdHeader)
{
    // Try to catck start of the command response (if response exists at all).
    char *_strCmd = strstr(_atCommandBuffer, _cmdHeader);

    // Command must be at the start of the response. Otherwise ignore it.
    if ((_strCmd != NULL) && ((_strCmd - _atCommandBuffer) < 2))
    {
        // Parse the id.
        int _id = 0;
        sscanf(_strCmd, "%d", &_id);

        // Move to the next part after the first comma.
        _strCmd = strchr(_strCmd, ',') + 1;

        // Parse the topic
        sscanf(_strCmd, "\"%[^\"]\"", _lastRxTopic);

        // Move to the next part after the second comma.
        _strCmd = strchr(_strCmd, ',') + 1;

        // Parse the size.
        sscanf(_strCmd, "%d", &_dataAvailable);

        // Check the size.
        if (_dataAvailable >= _maxRxBufferSize)
            _dataAvailable = _maxRxBufferSize - 1;

        // Move to the next part after the third comma.
        _strCmd = strchr(_strCmd, ',') + 1;

        // Parse the payload.
        strncpy(_rxDataBuffer, _strCmd, _dataAvailable);

        // Update pointer position.
        _currentPosition = _rxDataBuffer;
    }
}