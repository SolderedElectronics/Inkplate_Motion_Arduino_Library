// Include header file.
#include "esp32SpiAt.h"

// Flag for the handshake for the ESP32. Use SPI MODE0, MSBFIRST data transfet with approx. SPI clock rate of 20MHz.
static volatile bool _esp32HandshakePinFlag = false;

// SPI Settings for ESP32.
static SPISettings _esp32AtSpiSettings(20000000ULL, MSBFIRST, SPI_MODE0);

// ISR for the ESP32 handshake pin. This will be called automatically from the interrupt.
static void esp32HandshakeISR()
{
    _esp32HandshakePinFlag = true;
}

/**
 * @brief Construct a new Wi-Fi Class:: Wi Fi Class object
 *
 */
WiFiClass::WiFiClass()
{
    // Empty...for now.
}

/**
 * @brief   Initializes ESP32-C3 Module. It powers up the module, sets it to factory
 *          settings, initializes WiFi radio and disables storing settings in NVM.
 *
 * @param   bool _resetSettings
 *          Setting this to true will reset all setting stored in NVM of the ESP32.
 *          This is optional parameter and by default, this parameter is set to true,
 *          but it can be overdriven.
 * @return  bool
 *          True - Initialization ok, ESP32 is ready.
 *          False - Initialization failed.
 */
bool WiFiClass::init(bool _resetSettings)
{
    // Set the hardware level stuff first.

    // Set the SPI pins.
    SPI.setMISO(INKPLATE_ESP32_MISO_PIN);
    SPI.setMOSI(INKPLATE_ESP32_MOSI_PIN);
    SPI.setSCLK(INKPLATE_ESP32_SCK_PIN);

    // Initialize Arduino SPI Library.
    SPI.begin();

    // Set handshake pin.
    pinMode(INKPLATE_ESP32_HANDSHAKE_PIN, INPUT_PULLUP);

    // Set interrupt on handshake pin.
    attachInterrupt(digitalPinToInterrupt(INKPLATE_ESP32_HANDSHAKE_PIN), esp32HandshakeISR, RISING);

    // Set SPI CS Pin.
    pinMode(INKPLATE_ESP32_CS_PIN, OUTPUT);

    // Disable ESP32 SPI for now.
    digitalWrite(INKPLATE_ESP32_CS_PIN, HIGH);

    // Set ESP32 power switch pin.
    pinMode(INKPLATE_ESP32_PWR_SWITCH_PIN, OUTPUT);

    // Try to power on the modem. Return false if failed.
    if (!power(true, _resetSettings))
        return false;

    // If everything went ok, return true.
    return true;
}

/**
 * @brief   Power up or powers down the ESP32 module.
 *
 * @param   bool _en
 *          true - Enable the ESP32 module.
 *          false - Disables the ESP32 module.
 * @param   bool _resetSettings
 *          Setting this to true will reset all setting stored in NVM of the ESP32.
 *          This is optional parameter and by default, this parameter is set to true,
 *          but it can be overdriven.
 * @return  bool
 *          true - Modem is successfully powered up.
 *          false - Modem failed to power up.
 */
bool WiFiClass::power(bool _en, bool _resetSettings)
{
    if (_en)
    {
        // Enable the power to the ESP32.
        digitalWrite(INKPLATE_ESP32_PWR_SWITCH_PIN, HIGH);

        // Wait a little bit for the ESP32 to boot up.
        delay(50);

        // Wait for the EPS32 to be ready. It will send a handshake to notify master
        // To read the data - "\r\nready\r\n" packet. Since the handshake pin pulled high
        // with the external resistor, we need to wait for the handshake pin to go low first,
        // then wait for the proper handshake event.
        if (!isModemReady())
            return false;

        // Try to ping modem. Return fail if failed.
        if (!modemPing())
            return false;

        // Set ESP32 to its factory settings if needed.
        if (_resetSettings)
        {
            if (!systemRestore())
                return false;
        }

        // Disable echo on command. Return false if failed.
        if (!commandEcho(false))
            return false;

        // Enable default message filters.
        if (!defaultMsgFiltersEn()) return false;

        // Disable stroing data in NVM. Return false if failed.
        if (!storeSettingsInNVM(false))
            return false;

        // Disable system messages. Can disrupt flow of the library.
        if(!systemMessages(0))
            return false;

        // Initialize WiFi radio.
        if (!wiFiModemInit(true))
            return false;
    }
    else
    {
        // Disable the power to the ESP32.
        digitalWrite(INKPLATE_ESP32_PWR_SWITCH_PIN, HIGH);

        // Wait a little bit for the ESP32 to power down.
        delay(50);
    }

    // Everything went ok? Return true.
    return true;
}

/**
 * @brief   Methods sends AT command to the modem. It check if the modem is ready to accept the command or not.
 *
 * @param   char *_atCommand
 *          AT commnds that will be sent to the modem.
 * @param   uint16_t _len
 *          If the array is not null terminated, you can set the length parameter.
 *          This parameter is optional. If not set, array will be treated like nul terminated char array.
 * @return  bool
 *          true - AT Command is successfully sent.
 *          false - AT Command send failed (modem not ready to accept the command).
 * @note    AT Command needs to have CRLF at the and. method won't at it at the end of the command.
 */
bool WiFiClass::sendAtCommand(char *_atCommand, uint16_t _len)
{
    // Flush ESP32 from data read request if needed and if this is enabled, since
    // this can block EPS32 from receving new AT commands.
    flushModemReadReq();

    // Get the data size. Use lenght parameter if array is not a nul-terminated string.
    uint16_t _dataLen = _len != 0?_len:strlen(_atCommand);

    // First make a request for data send.
    dataSendRequest(_dataLen, 0);

    // Read the slave status.
    uint8_t _slaveStatus = 0;
    _slaveStatus = requestSlaveStatus();

    // Check the slave status, if must be INKPLATE_ESP32_SPI_SLAVE_STATUS_WRITEABLE.
    if (_slaveStatus != INKPLATE_ESP32_SPI_SLAVE_STATUS_WRITEABLE)
        return false;

    // Send the data.
    dataSend(_atCommand, _dataLen);

    // Send data end.
    dataSendEnd();

    return true;
}

/**
 * @brief   Methods waits the response from the ESP32. It check if the modem is
 *          requesting the data read from slave. After it received request,
 *          timeout triggers if the new data is not available after timeout value.
 *          Timeout time is measured after the last received packet or char.
 *
 * @param   char *_response
 *          Buffer where to store response.
 * @param   uint32_t _bufferLen
 *          length of the buffer for the response (in bytes, counting the null-terminating char).
 * @param   unsigned long _timeout
 *          Timeout value from the last received char or packet in milliseconds.
 * @return  bool
 *          true - Response has been received (no error handle for now).
 */
bool WiFiClass::getAtResponse(char *_response, uint32_t _bufferLen, unsigned long _timeout, uint16_t *_rxLen)
{
    // Timeout variable.
    unsigned long _timeoutCounter = 0;

    // Variable for the response array index offset.
    uint32_t _resposeArrayOffset = 0;

    // Capture the time!
    _timeoutCounter = millis();

    // Now loop until the timeout occurs
    while ((unsigned long)(millis() - _timeoutCounter) < _timeout)
    {
        // Wait for the response by checking the handshake pin.
        if (_esp32HandshakePinFlag)
        {
            // Read the slave status.
            uint16_t _responseLen = 0;
            uint8_t _slaveStatus = requestSlaveStatus(&_responseLen);

            // Check the slave status, if must be INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE
            if (_slaveStatus == INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE)
            {
                // Update the timeout!
                _timeoutCounter = millis();

                // Check if there is enough free memory in the buffer. If there is still free memory,
                // get the response. Otherwise, drop everything.
                if ((_responseLen + _resposeArrayOffset) < _bufferLen)
                {
                    // Read the data.
                    dataRead((_response + _resposeArrayOffset), _responseLen);

                    // Move the index in response array.
                    _resposeArrayOffset += _responseLen;
                }

                // Send read done.
                dataReadEnd();

                // Clear the flag.
                _esp32HandshakePinFlag = false;
            }
        }
    }

    // Add null-terminating char.
    _response[_resposeArrayOffset] = '\0';

    // If poiter is provided, pass number of received bytes.
    if (_rxLen != NULL) *_rxLen = _resposeArrayOffset;

    // Check if any data is received. If not, return false.
    return (_resposeArrayOffset != 0?true:false);
}

/**
 * @brief   Wait for the reponse form the modem. Method check if the modem is requesting a read from the
 *          master device. It will wait timeout value until for the response.
 *
 * @param   char *_response
 *          Buffer where to store response.
 * @param   uint32_t _bufferLen
 *          length of the buffer for the response (in bytes, counting the null-terminating char).
 * @param   unsigned long _timeout
 *          Timeout value until the packets start arriving.
 * @param   uint16_t *_rxLen
 *          Pointer to the variable ehere length of the receiveds data will be stored.
 * @return  bool
 *          true - Response has been received (no error handle for now).
 */
bool WiFiClass::getSimpleAtResponse(char *_response, uint32_t _bufferLen, unsigned long _timeout, uint16_t *_rxLen)
{
    // Timeout variable.
    unsigned long _timeoutCounter = 0;

    // Variable for the response string length.
    uint16_t _responseLen = 0;

    // Capture the time!
    _timeoutCounter = millis();

    // Now loop until the timeout occurs
    while (((unsigned long)(millis() - _timeoutCounter) < _timeout) && (!_esp32HandshakePinFlag))
        ;

    // If the timeout occured, return false.
    if (!_esp32HandshakePinFlag)
        return false;

    // Otherwise read the data.
    // Check the slave status, if must be INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE
    uint8_t _slaveStatus = requestSlaveStatus(&_responseLen);

    // Check the slave status, if must be INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE
    if (_slaveStatus != INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE)
        return false;

    // Check if the buffer is large enough for the data.
    // If not, drop everything.
    if (_responseLen < _bufferLen)
    {
        // Read the data.
        dataRead(_response, _responseLen);
    }

    // Clear handshake pin.
    _esp32HandshakePinFlag = false;

    // Send read done.
    dataReadEnd();

    // Add null-terminating char if needed.
    if (_rxLen == NULL)
    {
        _response[_responseLen] = '\0';
    }
    else
    {
        *_rxLen = _responseLen;
    }

    // Check if any data is received. If not, return false.
    return (_responseLen != 0?true:false);
}

bool WiFiClass::sendAtCommandWithResponse(char *_atCommand, unsigned long _timeoutAtCommand, unsigned long _rxDataTimeoutAtCommand, char *_expectedResponseAtCmd, uint8_t _expectedResponsePosition, bool _terminateOnAtResponseError, char *_dataPart, uint16_t _dataPartLen, unsigned long _timeoutDataPart, char *_expectedResponseData, uint16_t *_len)
{
    // Return value.
    bool _retValue = false;

    // Variable to store response length from ESP32.
    uint16_t _respLen = 0;
  
    // Check if the _atCommand id not null. Is it is, then do not send the command.
    if (_atCommand != NULL)
    {
        // Send AT Command! If send failed (ESP32 SPI status == readable), return false.
        if (!sendAtCommand(_atCommand)) return false;
    }

    // If the timeout is not zero, get the respose. If there is no any kind of response in preddefined time (timeout) return false (maybe flush will needed before new AT Command send, but that feature can be activated within the libary).
    if (_timeoutAtCommand != 0)
    {
        // First wait for the any kind of response by using HANDSHAKE pin.
        unsigned long _timeoutCounter = millis();
        while (((unsigned long)(millis() - _timeoutCounter) <= _timeoutAtCommand) && (!getHandshakePinState()))

        // If there is no handshake pin activity, timeout occured, return false.
        if (((unsigned long)(millis() - _timeoutCounter) > _timeoutAtCommand))
        {
          Serial.print("[DEBUG] no handshake pin activity, abort! Timeout value: ");
          Serial.println((unsigned long)(millis() - _timeoutCounter), DEC);
          return false;
        }

        // If something is received, read the response.
        if (!getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, _rxDataTimeoutAtCommand, &_respLen)) return false;

        // Try to parse if expected response exists.
        if (_expectedResponseAtCmd != NULL)
        {
            // Add null terminating char at the end (needed for string manipulating functions).
            _dataBuffer[_respLen] = '\0';
            
            // Calculate the position of start of the buffer.
            char *_bufferStart = NULL;
            switch (_expectedResponsePosition)
            {
                case INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START:
                  _bufferStart = _dataBuffer;
                  break;
                case INKPLATE_ESP32_AT_EXPECTED_RESPONSE_ANY:
                  _bufferStart = _dataBuffer;
                  break;
                case INKPLATE_ESP32_AT_EXPECTED_RESPONSE_END:
                  _bufferStart = _dataBuffer + _respLen - strlen(_expectedResponseAtCmd) - 5;
                  break;
                default:
                  _bufferStart = _dataBuffer;
            }
          
            // Save the first match occurance position.
            char *_responseMatch = strstr(_dataBuffer, _expectedResponseAtCmd);
    
            // If substring function returns NULL, match is not found.
            if (strstr(_dataBuffer, _expectedResponseAtCmd) == NULL)
            {
                // If flag for termination on unexpected response is set, return from the function.
                if (_terminateOnAtResponseError) return false;
            }
            else
            {
                // Ok, something is found. Check position in the case of the INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START.
                if ((_expectedResponsePosition == INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START) && ((_responseMatch - _bufferStart) > 5))
                {
                    // If flag for termination on unexpected response is set, return from the function.
                    if (_terminateOnAtResponseError) return false;
                }

                // Heh, it this is so simple. Sometimes, modem can respond with the command but also with the OK or ERROR after that command.
                // It's ok if there is no reponse, but it there is response and it's not ok, something is wrong!
                if (getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 200ULL, &_respLen))
                {
                    // Check of OK. If not found, return error.
                    if (strstr(_dataBuffer, "\r\nOK\r\n") == NULL) _retValue = false;
                }
    
                // Everything is ok with the response!
                _retValue = true;
            }
        }
    }

    // Now the data part. Only if the _dataPart is not null.
    if (_dataPart != NULL)
    {
        // Check the _dataPartLen. If is zero, then _dataPart is string. Use strlen to get the lenght of the string.
        if (_dataPartLen == 0) _dataPartLen = strlen (_dataPart);

        // Send AT Command! If send failed (ESP32 SPI status == readable), return false.
        if (!sendAtCommand(_dataPart, _dataPartLen)) return false;

        // If the timeout is not zero, get the respose. If there is no any kind of response in preddefined time (timeout) return false (maybe flush will needed before new AT Command send, but that feature can be activated within the libary).
        if (_timeoutDataPart)
        {
            // Ok, now wait for the response. This can be tricky since some commands sends additional OK after successful AT command (ex. +ATXYZ\r\n [delay] \r\nOK\r\n).
            if (!getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, _timeoutDataPart, &_respLen)) return false;

            if (_expectedResponseData != NULL)
            {
                // Check for the expected response.
                if (strstr(_dataBuffer, _expectedResponseData) == NULL) return false;

                // Ok, is something is found, check for additional OK/ERROR.
                if (getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 200ULL, &_respLen))
                {
                    // Check of OK. If not found, return error.
                    if (strstr(_dataBuffer, "\r\nOK\r\n") == NULL) _retValue = false;
                }
            }

            // You got so far. Everything seems to went fine, so return true!
            _retValue = true;
        }
    }

    // In needed, copy response len.
    if (_len != NULL) *_len = _respLen;

    // Return success/fail flag.
    return _retValue;
}

/**
 * @brief   Send AT Command + Data part
 * 
 * @param   char *_atCommand
 *          AT Command (must contain \r\n at the end).
 * @param   char *_dataPart
 *          Data part of the command (that needs to be sent after ">")
 * @param   uint16_t _dataPartLen
 *          Lenght of the data part (set 0 is _dataPart if null terminated string).
 * @param   unsigned long _timeout
 *          Timeout on response on AT Command after the data is sent in milliseconds.
 * @param   char *_expectedResponse
 *          response from the modem if the command and data successfully accepted. By default, this parameter is set to
 *          "\r\nOK\r\n".
 * @return  bool
 *          true - Command and data are accepted.
 *          false - Command or data failed.
 */
bool WiFiClass::sendDataPart(char *_atCommand, char *_dataPart, uint16_t _dataPartLen, unsigned long _timeout, char *_expectedResponse)
{
    if (_dataPart != NULL)
    {
        // If lenght is 0, it means this is string.
        if (_dataPartLen == 0) _dataPartLen = strlen(_dataPart);

        // Not a empty data set or string.
        if (_dataPartLen > 0)
        {
            // Send AT Command!
            if (sendAtCommand(_atCommand)) return false;
            // Get the resposne (should response with ">").
            if (getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 20ULL)) return false;
            // Check for the response.
            if (strstr(_dataBuffer, "\r\n>") == NULL) return false;
            // Send the client ID itself.
            if (sendAtCommand(_dataPart, _dataPartLen)) return false;
            // Now wait for the proper AT response.
            if (getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, _timeout));
            // Check for the "OK". If is missing from the response, return error.
            if (strstr(_dataBuffer, _expectedResponse) == NULL) return false;
        }
    }

    // Return true for success.
    return true;
}

bool WiFiClass::messageFilter(bool _enable, char* _headFilter, char *_tailFilter)
{
    // Return value.
    bool _retValue = false;
  
    // Check the each size of the filter.
    int _headFilterLen = 0;
    int _tailFilterLen = 0;
    
    if (_headFilter != NULL) _headFilterLen = strlen(_headFilter);
    if (_tailFilter != NULL) _tailFilterLen = strlen(_tailFilter);

    // Send AT Command for message filter. If the filter should be enabled, first parameter must be 1, otherwise if needs to be removed set it to 2.
    sprintf(_dataBuffer, "AT+SYSMSGFILTERCFG=%d,%d,%d\r\n", _enable?1:2, _headFilterLen, _tailFilterLen);
    
    // Send AT Command! If send failed (ESP32 SPI status == readable), return false.
    if (!sendAtCommand(_dataBuffer)) return false;

    // Now wait for the response. It should send "\r\nOK\r\n\r\n>".
    if (!getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 100ULL)) return false;

    // Check for the response.
    char *_match = strstr(_dataBuffer, "\r\nOK\r\n\r\n>");

    // It must be at the start of the response.
    if (_match != NULL)
    {
        if ((_match - _dataBuffer) > 5) return false;
    }
    else
    {
        // No expected response is found, return false.
        return false;
    }

    // First try to send head if needed.
    if (_headFilter != NULL)
    {
        // If failed, return false.
        if (!sendAtCommand(_headFilter)) return false;

        // Must be delay!
        delay(5);
    }

    // Now try to send tail if needed.
    if (_tailFilter != NULL)
    {
        // If failed, return false.
        if (!sendAtCommand(_tailFilter)) return false;

        // No delay this time.
    }

    // Wait for the response. It should send "\r\nOK\r\n".
    if (!getSimpleAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 100ULL)) return false;

    // Check for the response.
    _match = strstr(_dataBuffer, "\r\nOK\r\n");

    // Match must exits!
    if (_match != NULL) _retValue = true;
  
    // Return success flag.
    return _retValue;
}

/**
 * @brief   Method pings the modem (sends "AT" command and waits for AT OK).
 *
 * @return  bool
 *          true - Modem is available (received AT OK).
 *          false - Modem is not available.
 */
bool WiFiClass::modemPing()
{
    // Send "AT" AT Command for Modem Ping.
    sendAtCommand((char *)esp32AtPingCommand);

    // Wait for the response from the modem.
    if (!getAtResponse((char *)_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 20ULL))
        return false;

    // Check if AT\r\n\r\nOK\r\n\r\n is received.
    if (strcmp((char *)_dataBuffer, esp32AtPingResponse) != 0)
        return false;

    // If everything went ok, return true.
    return true;
}

/**
 * @brief   Methods sends AT command to set factory settings to the ESP32.
 *
 * @return  bool
 *          true - Modem restored settings to it's factory values.
 *          false - Modem failed to restore settings.
 */
bool WiFiClass::systemRestore()
{
    // Calling this command will set ESP32 to its factory settings.
    if (!sendAtCommand((char *)esp32AtCmdSystemRestore))
        return false;

    // ESP32 nonsense. Wait for 2 seconds after memory restore.
    // Using handshake doesn't work reliably since it fires multiple times.
    delay(2200);

    // Read a response to clear the flag.
    if (!getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 40ULL)) return false;

    // Everything went ok? Return true.
    return true;
}

/**
 * @brief   Enable or disable storing settings into ESP32 NVM. By default, while power up, it is
 *          disabled.
 *
 * @param   bool _store
 *          true - Store settings into ESP32 NVM.
 *          false - Do not store settings into NVM.
 * @return  bool
 *          true - Command executed successfully.
 *          false - Command failed.
 */
bool WiFiClass::storeSettingsInNVM(bool _store)
{
    // Disable or enable storing data in NVM. By default, storing is enabled, but at the ESP
    // start up is disabled.

    // Make a AT Command depending on the choice of storing settings in NVM.
    sprintf(_dataBuffer, "AT+SYSSTORE=%d\r\n", _store ? 1 : 0);

    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (!sendAtCommandWithResponse(_dataBuffer, 20ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Get the pointer (address) of the buffer for the SPI data.
 *
 * @return  char*
 *          Pointer of the SPI buffer for ESP32 communication and commands.
 */
char *WiFiClass::getDataBuffer()
{
    // Return the main RX/TX buffer fopr SPI data.
    return _dataBuffer;
}

/**
 * @brief   Disable or enable system prompt messages. By default,
 *          they are all disabled since they interfere with the library.
 * @param   uint8_t _cfg
 *          Config for the system messages (see desc. of AT+SYSMSG)
 * @return  bool
 *          true - Command execution was successfull.
 *          false - Command set failed.
 */
bool WiFiClass::systemMessages(uint8_t _cfg)
{
    // Make AT command depending on the confing.
    sprintf(_dataBuffer, "AT+SYSMSG=%d\r\n", _cfg);

    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (!sendAtCommandWithResponse(_dataBuffer, 20ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Return true if everything is ok.
    return true;
}

/**
 * @brief   Method enables or disables flushing ESP32 from data read requests before
 *          AT Command send.
 * 
 * @param   bool _en
 *          true - Flush the ESP32 before every AT Command send.
 *          false - Do not flush ESP32 from data read request before AT command send. 
 */
void WiFiClass::flushBeforeCommand(bool _en)
{
    // Store the state of the setting internally.
    _flushEspBeforeCmdSend = _en;
}

bool WiFiClass::defaultMsgFiltersEn()
{
    // Enable message filter for the WiFi Connect.
    if (!messageFilter(true, "^WIFI CONNECTED\r\n", NULL)) return false;

    // Enable message filter for the WiFi Disconnect.
    if (!messageFilter(true, "^WIFI DISCONNECT\r\n", NULL)) return false;

    // Enable message filter for the WIFI GOT IP message.
    if (!messageFilter(true, "^WIFI GOT IP\r\n", NULL)) return false;

    // Enable system message filters. Return false if failed.
    if (!systemMsgFiltering(true)) return false;

    // If code got to here, everything seems ok, return true for success.
    return true;
}

bool WiFiClass::systemMsgFiltering(bool _en)
{
    // Create AT Command with sprintf.
    sprintf(_dataBuffer, "AT+SYSMSGFILTER=%d\r\n", _en?1:0);

    // Send AT command and wait for the respose. If there is no expected response, return false;
    if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 5ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Methods sets WiFi mode (null, station, SoftAP or station and SoftAP).
 *
 * @param   uint8_t _wifiMode
 *          Use preddifend macros (can be found in WiFiSPITypedef.h)
 *          INKPLATE_WIFI_MODE_NULL - Null mode (modem disabled)
 *          INKPLATE_WIFI_MODE_STA - Station mode
 *          INKPLATE_WIFI_MODE_AP - Soft Access Point (not implemeted yet!)
 *          INKPLATE_WIFI_MODE_STA_AP - Both station and Soft Access Point (AP mode still not implemeted!).
 * @return  bool
 *          true - Mode set successfully on ESP32
 *          false - Mode set failed.
 */
bool WiFiClass::setMode(uint8_t _wifiMode)
{
    // Check for the proper mode.
    if ((_wifiMode > INKPLATE_WIFI_MODE_STA_AP))
        return false;

    // First disconnect from the current network.
    disconnect();

    // Create AT Command string depending on the mode.
    sprintf(_dataBuffer, "AT+CWMODE=%d\r\n", _wifiMode);

    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise, return ok.
    return true;
}

/**
 * @brief   Connect to the access point.
 *
 * @param   char *_ssid
 *          Char array/pointer to the AP name name.
 * @param   char *_pass
 *          Char array/pointer to the AP password.
 * @return  bool
 *          true - Command execution was successfull.
 *          false -  Command execution failed.
 * @note    Max characters for password is limited to 63 chars and SSID is only limited to UTF-8 encoding.
 *          Try to avoid usage following chars: {"}, {,}, {\\}. If used, escape char must be added.
 */
bool WiFiClass::begin(char *_ssid, char *_pass)
{
    // Check for user mistake (null-pointer!).
    if ((_ssid == NULL) || (_pass == NULL))
        return false;

    // Create string for AT comamnd.
    sprintf(_dataBuffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", _ssid, _pass);

    // Send AT command.
    if (!sendAtCommand(_dataBuffer))
        return false;

    // Set the flag for WiFi connection in progress.
    _wifiConnectionInProgres = true;

    // Return ture - no need to be here, just future proof.
    return true;
}

/**
 * @brief   Methods returns the status of the ESP32 WiFi connection the AP.
 *
 * @return  bool
 *          true - ESP32 is connected to the AP.
 *          false - ESP32 is not connected to the AP.
 */
bool WiFiClass::connected()
{
    // Check if the WiFi connection to the AP is currently in progress.
    if (_wifiConnectionInProgres)
    {
        // That means EPS32 will send WL CONNECTED message as soon as the connection with the AP
        // is established.
        if (getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 100ULL))
        {
            // If you got something, check if is the message for the WiFi.
            if (strstr(_dataBuffer, esp32AtCmdResponseOK))
            {
                // OK, now we got access to the AP.
                _wifiConnectionInProgres = false;

                // Return true for succesfully connected to the AP.
                return true;
            }
        }
    }
    else
    {
        // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
        if (sendAtCommandWithResponse((char*)"AT+CWSTATE?\r\n", 200ULL, 4ULL, "+CWSTATE:", INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL))
        {
            // Parse the data.
            int _result;
            if (sscanf(_dataBuffer, "+CWSTATE:%d", &_result) != 1)
                return false;

            // If the result is 2, that means it's connected to the WiFi and ESP32 got the IP address.
            if (_result == 2)
                return true;
        }
        else
        {
            return false;
        }
    }

    // No response? Return false.
    return false;
}
/**
 * @brief   Method executes command to the ESP32 to disconnects from the AP.
 *
 * @return  bool
 *          true - Command is executed successfully.
 *          false - Command failed.
 */
bool WiFiClass::disconnect()
{
    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (!sendAtCommandWithResponse((char*)esp32AtWiFiDisconnectCommand, 2000ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise, return ok.
    return true;
}

/**
 * @brief   Methods prompts ESP32 to run a WiFi network scan. Scan takes about 2 seconds.
 *
 * @return  int
 *          Number of available networks (including encrypted and hidden ones).
 */
int WiFiClass::scanNetworks()
{
    // Issue a WiFi Scan command and get the response. Check if the response is expected. Otherwise, return false.
    if (sendAtCommandWithResponse((char*)esp32AtWiFiScan, 2000ULL, 4ULL, (char*)("+CWLAP:"), INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL))
    {
        // Find first occurance of the first found network.
        char *_wifiAPStart = strstr(_dataBuffer, "+CWLAP:");

        // Parse how many networks have been found.
        while ((_wifiAPStart != NULL) && (_foundWiFiAp < INKPLATE_ESP32_MAX_SCAN_AP))
        {
            _startApindex[_foundWiFiAp] = _wifiAPStart - _dataBuffer;
            _foundWiFiAp++;
            _wifiAPStart = strstr(_wifiAPStart + 1, "+CWLAP:");
        }
    }
    else
    {
        // No reponse? Return 0 found neworks.
    }

    // Otherwise, if some networks werw found, return number of available networks.
    return _foundWiFiAp;
}

/**
 * @brief   Method gets SSID of a scaned network.
 *
 * @param   int _ssidNumber
 *          Network number on the found network list.
 * @return  char*
 *          SSID of the network.
 */
char *WiFiClass::ssid(int _ssidNumber)
{
    // Check if the parsing is successfull. If not, return empty string.
    if (!parseFoundNetworkData(_ssidNumber, &_lastUsedSsid, &_lastUsedSsidData))
        return "\0";

    // If parsing is successfull, return SSID name.
    return _lastUsedSsidData.ssidName;
}

/**
 * @brief   Method gets auth. status of the selected network after the scan.
 *
 * @param   int _ssidNumber
 *          Network number on the found network list.
 * @return  bool
 *          true - Network is encrypted.
 *          false - Network is open.
 */
bool WiFiClass::auth(int _ssidNumber)
{
    // Parse the found network data.
    parseFoundNetworkData(_ssidNumber, &_lastUsedSsid, &_lastUsedSsidData);

    // If parsing is successfull, return auth status (false = open network, true = password locked".
    return _lastUsedSsidData.authType ? true : false;
}

/**
 * @brief   Method gets the RRIS value of the selected scaned networtk.
 *
 * @param   int _ssidNumber
 *          Network number on the found network list.
 * @return  int
 *          RSSI value in dBm of the selected network.
 */
int WiFiClass::rssi(int _ssidNumber)
{
    // Parse the found network data.
    parseFoundNetworkData(_ssidNumber, &_lastUsedSsid, &_lastUsedSsidData);

    // If parsing is successfull, return RSSI.
    return _lastUsedSsidData.rssi;
}

/**
 * @brief   Obtain a local IP.
 *
 * @return  IPAddress
 *          Returns the local IP with Arduino IPAddress class.
 */
IPAddress WiFiClass::localIP()
{
    return ipAddressParse("ip");
}

/**
 * @brief   Get a gateway IP Address.
 *
 * @return  IPAddress
 *          Returns the Gateway IP with Arduino IPAddress class.
 */
IPAddress WiFiClass::gatewayIP()
{
    return ipAddressParse("gateway");
}

/**
 * @brief   Get a network subnet mask.
 *
 * @return  IPAddress
 *          Returns the network subnet mask with Arduino IPAddress class.
 */
IPAddress WiFiClass::subnetMask()
{
    return ipAddressParse("netmask");
}

/**
 * @brief   Get the DNS of the primary or secondary DNS.
 *
 * @param   uint8_t i
 *          0 - Get the DNS IP Address of the primary DNS.
 *          1 - Get the DNS IP Address of the secondary DNS.
 * @return  IPAddress
 *          Returns the selected DNS IP Address with Arduino IPAddress class.
 */
IPAddress WiFiClass::dns(uint8_t i)
{
    // Filter out the selected DNS. It can only be three DNS IP Addreses.
    if (i > 2)
        return INADDR_NONE;

    // DNS IP addresses array.
    int _dnsIpAddresses[3][4];

    // Clear the array.
    memset(_dnsIpAddresses[0], 0, sizeof(int) * 4);
    memset(_dnsIpAddresses[1], 0, sizeof(int) * 4);
    memset(_dnsIpAddresses[2], 0, sizeof(int) * 4);

    // Flag if the static or dynamic IP is used on ESP32.
    int _dhcpFlag = 0;

    // Issue a AT Command for DNS. Return invalid IP address if failed.
    if (sendAtCommandWithResponse((char*)esp32AtGetDns, 200ULL, 4ULL, (char*)("+CIPDNS:"), INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL))
    {
        // Try to parse the data.
        char *_responseStart = strstr(_dataBuffer, "+CIPDNS:");

        // If not found, return invalid IP Address.
        if (_responseStart == NULL)
            INADDR_NONE;

        // Parse it!
        int _res = sscanf(_responseStart, "+CIPDNS:%d,\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"", &_dhcpFlag,
                        &_dnsIpAddresses[0][0], &_dnsIpAddresses[0][1], &_dnsIpAddresses[0][2], &_dnsIpAddresses[0][3],
                        &_dnsIpAddresses[1][0], &_dnsIpAddresses[1][1], &_dnsIpAddresses[1][2], &_dnsIpAddresses[1][3],
                        &_dnsIpAddresses[2][0], &_dnsIpAddresses[2][1], &_dnsIpAddresses[2][2], &_dnsIpAddresses[2][3]);

        // Check if all is parsed correctly, it should find at least one DNS. If not, return invalid IP Address.
        if (_res < 4)
            return INADDR_NONE;
    }
    else
    {
        // Failed to send / get response? Return invalid IP Address.
        return INADDR_NONE;
    }

    // Return wanted DNS.
    return IPAddress(_dnsIpAddresses[i][0], _dnsIpAddresses[i][1], _dnsIpAddresses[i][2], _dnsIpAddresses[i][3]);
}

/**
 * @brief   Get the MAC address of the ESP32. It will be returned with char array
 *          ("aa:bb:cc:11:22:33").
 *
 * @return  char*
 *          MAC Address of the ESP32.
 * @note    Original MAC can be changed with WiFiClass::macAddress(char *_mac) method.
 */
char *WiFiClass::macAddress()
{
    // Send AT Command for getting AT Command from the module.
    if (sendAtCommandWithResponse((char*)esp32AtWiFiGetMac, 200ULL, 4ULL, (char*)("+CIPAPMAC:"), INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL))
    {
        // Try to parse it.
        char *_responseStart = strstr(_dataBuffer, "+CIPAPMAC:");

        // If proper response is not found, return with invalid MAC address.
        if (_responseStart == NULL)
            return _invalidMac;

        // Get the MAC address from the response.
        int _res = sscanf(_responseStart, "+CIPAPMAC:%[^\r\n]", _esp32MacAddress);

        // If parsing failed, return invalid MAC address.
        if (!_res)
            return _invalidMac;
    }
    else
    {
        // If command or response failed return invalid MAC address.
        return _invalidMac;
    }

    // Return parsed MAC address.
    return _esp32MacAddress;
}

/**
 * @brief   Method set the MAC address of the ESP32.
 *
 * @param   char *_mac
 *          Pointer to the char array of the new MAC address. String with the
 *          new MAC address must have "aa:bb:cc:dd:ee:ff" format.
 * @return  bool
 *          true - New MAC address is set.
 *          false - New MAC address set failed.
 * @note    MAC address only can be set if the WiFI mode is set to SoftAP mode.
 */
bool WiFiClass::macAddress(char *_mac)
{
    // Create a string for the new MAC address.
    sprintf(_dataBuffer, "AT+CIPAPMAC=\"%s\"\r\n", _mac);

    // Send AT commands and check for the expected response ("\r\nOK\r\n"). If sending AT command failed or there is no response, return false indicating error.
    if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Methods enables complete WiFi config. Set LocalIP, GatewayIP, Subnet Mask and DNS with one call.
 *          To keep original value of the one IP address, use INADDR_NONE as parameter.
 *
 * @param   IPAddress _staticIP
 *          Set the local IP Address. To keep the default one, use INADDR_NONE.
 * @param   IPAddress _gateway
 *          Set the gateway IP Address. To keep the default one, use INADDR_NONE.
 * @param   IPAddress _subnet
 *          Set the sunbet mask. To keep the default one, use INADDR_NONE.
 * @param   IPAddress _dns1
 *          Set the Primary DNS IP Address. To keep the default one, use INADDR_NONE.
 * @param   IPAddress _dns2
 *          Set the Secondary DNS IP Address. To keep the default one, use INADDR_NONE.
 * @return  bool
 *          true - New IP config is set.
 *          false - New IP config set failed.
 */
bool WiFiClass::config(IPAddress _staticIP, IPAddress _gateway, IPAddress _subnet, IPAddress _dns1, IPAddress _dns2)
{
    // First get the current settings since not all of above must be included.
    if (_staticIP == INADDR_NONE)
    {
        _staticIP = ipAddressParse("ip");
    }

    if (_gateway == INADDR_NONE)
    {
        _gateway = ipAddressParse("gateway");
    }

    if (_subnet == INADDR_NONE)
    {
        _subnet = ipAddressParse("netmask");
    }

    if (_dns1 == INADDR_NONE)
    {
        _dns1 = dns(0);
    }

    if (_dns1 == INADDR_NONE)
    {
        _dns1 = dns(1);
    }

    // Now send modified data.
    // Check if anything with the IP config have been modified. If so, send new settings.
    if ((_staticIP != INADDR_NONE) || (_gateway != INADDR_NONE) || (_subnet != INADDR_NONE))
    {
        // Send the AT commands for the new IP config.
        sprintf(_dataBuffer, "AT+CIPSTA=\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"\r\n", _staticIP[0],
                _staticIP[1], _staticIP[2], _staticIP[3], _gateway[0], _gateway[1], _gateway[2], _gateway[3],
                _subnet[0], _subnet[1], _subnet[2], _subnet[3]);

        // Send AT command, wait for the response.
        if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;
    }

    // Check the same thing, but for DNS.
    if ((_dns1 != INADDR_NONE) || (_dns2 != INADDR_NONE))
    {
        // Create AT command for the DNS settings.
        sprintf(_dataBuffer, "AT+CIPDNS=1,\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"\r\n", _dns1[0], _dns1[1], _dns1[2], _dns1[3],
                _dns2[0], _dns2[1], _dns2[2], _dns2[3]);

        // Send AT command, wait for the response.
        if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;
    }

    // If the code got so far, everything went ok, return true for success.
    return true;
}

/**
 * @brief   Wait for the ESP32 handshake pin to trigger master request using digitalRead()
 *          (polling). This funciton is not used anymore.
 *
 * @param   uint32_t _timeoutValue
 *          Timeout value until the request from the ESP32 happens in milliseconds.
 * @param   _validState
 *          Is the trigger state for the handshake pin HIGH or LOW.
 * @return  bool
 *          true - Handshake pin trigger detected.
 *          false - Timeout, no handshake trigger detected.
 */
bool WiFiClass::waitForHandshakePin(uint32_t _timeoutValue, bool _validState)
{
    // Variable for the timeout. Also capture the current state.
    unsigned long _timeout = millis();

    // Read the current state of the handshake pin.
    bool _handshakePinState = digitalRead(INKPLATE_ESP32_HANDSHAKE_PIN);

    // Check if the handshake pin is already set.
    if (_handshakePinState == _validState)
        return true;

    // If not, wait for the valid pin state.
    do
    {
        // Read the new state of the pin.
        _handshakePinState = digitalRead(INKPLATE_ESP32_HANDSHAKE_PIN);

        // Wait a little bit.
        delay(1);
    } while (((unsigned long)(millis() - _timeout) < _timeoutValue) && (_handshakePinState != _validState));

    // Check the state of the timeout. If timeout occured, return false.
    if ((millis() - _timeout) >= _timeoutValue)
        return false;

    // Otherwise return true.
    return true;
}

bool WiFiClass::getHandshakePinState()
{
    return _esp32HandshakePinFlag;
}

/**
 * @brief   Wait for the ESP32 handshake pin to trigger master request using interrutps.
 *
 * @param   uint32_t _timeoutValue
 *          Timeout value until the request from the ESP32 happens in milliseconds.
 * @return  bool
 *          true - Handshake pin trigger detected.
 *          false - Timeout, no handshake trigger detected.
 */
bool WiFiClass::waitForHandshakePinInt(uint32_t _timeoutValue)
{
    // First, clear the flag status.
    _esp32HandshakePinFlag = false;

    // Variable for the timeout. Also capture the current state.
    unsigned long _timeout = millis();

    // Wait for the rising edge in Handshake pin.
    while (((unsigned long)(millis() - _timeout) < _timeoutValue) && (!_esp32HandshakePinFlag))
        ;

    // Clear the flag.
    _esp32HandshakePinFlag = false;

    // Check the state of the timeout. If timeout occured, return false.
    if ((millis() - _timeout) >= _timeoutValue)
        return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Get ESP32 slave status. It is used right after the ESP32 sends handshake (ESP32 issues
 *          request from the master to read the data from the ESP32).
 *
 * @param   uint16_t _len
 *          Number of bytes requested waiting to be read by thje master from the ESP32.
 * @return  uint8_t
 *          Return slave status (INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE or
 *          INKPLATE_ESP32_SPI_SLAVE_STATUS_WRITEABLE).
 */
uint8_t WiFiClass::requestSlaveStatus(uint16_t *_len)
{
    // Make a union/struct for data part of the SPI Command.
    union spiAtCommandsSlaveStatusTypedef _slaveStatus;
    _slaveStatus.elements.status = 0;
    _slaveStatus.elements.sequence = 0;
    _slaveStatus.elements.length = 0;

    // SPI Command Packet.
    struct spiAtCommandTypedef _spiCommand = {
        .cmd = INKPLATE_ESP32_SPI_CMD_REQ_SLAVE_INFO,
        .addr = 0x04,
        .dummy = 0x00,
        .data = (uint8_t *)&(_slaveStatus.bytes),
    };

    uint32_t _spiPacketLen = sizeof(_slaveStatus.bytes);

    // Transfer the packet!
    transferSpiPacket(&_spiCommand, _spiPacketLen);

    // Save the length if possible.
    if (_len != NULL)
        *_len = _slaveStatus.elements.length;

    // Return the slave status.
    return _slaveStatus.elements.status;
}

/**
 * @brief   Send data to the ESP32 and at the same time receive new data from ESP32.
 *
 * @param   spiAtCommandTypedef *_spiPacket
 *          Pointer to the spiAtCommandTypedef to describe data packet.
 * @param   uint16_t _spiDataLen
 *          length of the data part only, excluding spiAtCommandTypedef (in bytes).
 */
void WiFiClass::transferSpiPacket(spiAtCommandTypedef *_spiPacket, uint16_t _spiDataLen)
{
    // Get the SPI STM32 HAL Typedef Handle.
    SPI_HandleTypeDef *_spiHandle = SPI.getHandle();

    // Activate ESP32 SPI lines by pulling CS pin to low.
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);

    // Send everything, but the data.
    SPI.beginTransaction(_esp32AtSpiSettings);
    SPI.transfer(_spiPacket->cmd);
    SPI.transfer(_spiPacket->addr);
    SPI.transfer(_spiPacket->dummy);

    // SPI.transfer(_spiPacket->data, _spiDataLen);
    HAL_SPI_TransmitReceive(_spiHandle, (uint8_t *)_spiPacket->data, (uint8_t *)_spiPacket->data, _spiDataLen,
                            HAL_MAX_DELAY);
    SPI.endTransaction();

    // Disable ESP32 SPI lines by pulling CS pin to high.
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
}

/**
 * @brief   Method only sends SPI data to the ESP32.
 *
 * @param   spiAtCommandTypedef *_spiPacket
 *          Pointer to the spiAtCommandTypedef to describe data packet.
 * @param   uint16_t _spiDataLen
 *          length of the data part only, excluding spiAtCommandTypedef (in bytes).
 */
void WiFiClass::sendSpiPacket(spiAtCommandTypedef *_spiPacket, uint16_t _spiDataLen)
{
    // Get the SPI STM32 HAL Typedef Handle.
    SPI_HandleTypeDef *_spiHandle = SPI.getHandle();

    // Pack ESP32 SPI Packer Header data.
    uint8_t _esp32SpiHeader[] = {_spiPacket->cmd, _spiPacket->addr, _spiPacket->dummy};

    // Activate ESP32 SPI lines by pulling CS pin to low.
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);

    // Send everything, but the data.
    SPI.beginTransaction(_esp32AtSpiSettings);
    HAL_SPI_Transmit(_spiHandle, _esp32SpiHeader, sizeof(_esp32SpiHeader) / sizeof(uint8_t), HAL_MAX_DELAY);

    // Send data.
    HAL_SPI_Transmit(_spiHandle, _spiPacket->data, _spiDataLen, HAL_MAX_DELAY);
    SPI.endTransaction();

    // Disable ESP32 SPI lines by pulling CS pin to high.
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
}

bool WiFiClass::commandEcho(bool _en)
{
    // Turn the Echo on or off.
    sprintf(_dataBuffer, "ATE%d\r\n", _en?1:0);
    if (!sendAtCommand(_dataBuffer))
        return false;
    if (!getAtResponse(_dataBuffer, INKPLATE_ESP32_AT_CMD_BUFFER_SIZE, 20ULL))
        return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Send data to the ESP32.
 *
 * @param   char *_dataBuffer
 *          Pointer to the data buffer.
 * @param   uint32_t _len
 *          length of the data (in bytes).
 * @return  bool
 *          true - Data sent successfully.
 */
bool WiFiClass::dataSend(char *_dataBuffer, uint32_t _len)
{
    // Before sending data:
    // 1. Make a request for data send
    // 2. Read and check slave status - It should return with INKPLATE_ESP32_SPI_SLAVE_STATUS_WRITEABLE.

    // Calculate the number of chunks to send, since the max is 4092 bytes.
    uint16_t _chunks = _len / INKPLATE_ESP32_SPI_MAX_MESAGE_DATA_BUFFER;
    uint16_t _lastChunk = _chunks ? _len % INKPLATE_ESP32_SPI_MAX_MESAGE_DATA_BUFFER : _len;

    // Address offset for the data packet.
    uint32_t _dataPacketAddrOffset = 0;

    // Create an data packet for data send.
    struct spiAtCommandTypedef _spiDataSend = {
        .cmd = INKPLATE_ESP32_SPI_CMD_MASTER_SEND, .addr = 0x00, .dummy = 0x00, .data = (uint8_t *)(_dataBuffer)};

    // Go trough the chunks.
    while (_chunks--)
    {
        // Calculate the chunk size.
        uint16_t _chunkSize = _chunks != 0 ? INKPLATE_ESP32_SPI_MAX_MESAGE_DATA_BUFFER : _lastChunk;

        // Transfer the data!
        sendSpiPacket(&_spiDataSend, _chunkSize);

        // Update the address position.
        _dataPacketAddrOffset += _chunkSize;

        // Update the SPI ESP32 packer header.
        _spiDataSend.data = (uint8_t *)(_dataBuffer + _chunkSize);

        // Data end cmd? Don't know...Needs to be checked!
    }

    // Write the last one chunk (or the only one if the _len < 4092).
    sendSpiPacket(&_spiDataSend, _lastChunk);

    // Return true for success.
    return true;
}

/**
 * @brief   Send command to the ESP32 indicating data send has ended.
 *
 * @return  bool
 *          true - Command send successfully.
 */
bool WiFiClass::dataSendEnd()
{
    // Create the structure for the ESP32 SPI.
    // Data field is not used here.
    struct spiAtCommandTypedef _spiDataSend = {.cmd = INKPLATE_ESP32_SPI_CMD_MASTER_SEND_DONE, .addr = 0, .dummy = 0};

    // Transfer the packet! The re is not data field this time, so it's size is zero.
    transferSpiPacket(&_spiDataSend, 0);

    // Return true for success.
    return true;
}

/**
 * @brief   Read the data requested by the ESP32 with handshake pin.
 *
 * @param   char *_dataBuffer
 *          Pointer to the data buffer (where to store received data).
 * @param   _len
 *          Number of bytes needed to be read from the ESP32 (get this data from the slave status).
 * @return  bool
 *          true - Data read ok.
 */
bool WiFiClass::dataRead(char *_dataBuffer, uint16_t _len)
{
    // Before reading the data:
    // 1. Make a request for data read.
    // 2. Read and check slave status - It should return with INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE.

    // Create an data packet for data send.
    struct spiAtCommandTypedef _spiDataSend = {
        .cmd = INKPLATE_ESP32_SPI_CMD_MASTER_READ_DATA, .addr = 0x00, .dummy = 0x00, .data = (uint8_t *)(_dataBuffer)};

    // Read the last one chunk (or the only one if the _len < 4092).
    transferSpiPacket(&_spiDataSend, _len);

    // Return true for success.
    return true;
}

/**
 * @brief   Send ESP32 that no more data will be read. Must be
 *          sent after data read.
 *
 * @return  bool
 *          true - Command sent successfully.
 */
bool WiFiClass::dataReadEnd()
{
    // Create the structure for the ESP32 SPI.
    // Data field is not used here.
    struct spiAtCommandTypedef _spiDataSend = {.cmd = INKPLATE_ESP32_SPI_CMD_MASTER_READ_DONE, .addr = 0, .dummy = 0};

    // Transfer the packet! The re is not data field this time, so it's size is zero.
    transferSpiPacket(&_spiDataSend, 0);

    // Return true for success.
    return true;
}

/**
 * @brief   Make a request to send data to the ESP32.
 *
 * @param   int _len
 * @param   _seqNumber
 *          Message sequnece number - Guess this is used if the message is chunked.
 * @return  bool
 *          true - Request sent successfully.
 */
bool WiFiClass::dataSendRequest(uint16_t _len, uint8_t _seqNumber)
{
    // Create the structure for the ESP32 SPI.
    // Data field data info field now (spiAtCommandDataInfoTypedef union).

    // First fill the spiAtCommandDataInfoTypedef union (aka. data info field in the data field).
    spiAtCommandDataInfoTypedef _dataInfo;
    _dataInfo.elements.magicNumber = INKPLATE_ESP32_SPI_DATA_INFO_MAGIC_NUM;
    _dataInfo.elements.sequence = _seqNumber;
    _dataInfo.elements.length = _len;

    struct spiAtCommandTypedef _spiDataSend = {
        .cmd = INKPLATE_ESP32_SPI_CMD_REQ_TO_SEND_DATA,
        .addr = 0,
        .dummy = 0,
        .data = (uint8_t *)&(_dataInfo.bytes),
    };

    // Transfer the packet! The re is not data field this time, so it's size is zero.
    transferSpiPacket(&_spiDataSend, sizeof(_dataInfo.bytes));

    // Wait for the handshake!
    bool _ret = waitForHandshakePinInt(200ULL);

    // Return the success status. If timeout occured, data read req. has failed.
    return _ret;
}

/**
 * @brief   Method flushes all data from the ESP32 if needed and if this feature is
 *          enabled. Enable or disable can be done with flushBeforeCommand(bool _en).
 *          Additional check for this setting doesn't have to be done, since this is
 *          already done in the method.
 * 
 * @return  bool
 *          true - Flush done (there was some kind of read data request from the ESP32).
 *          flase - No data read request from the ESP32 - flush did not happend.
 */
bool WiFiClass::flushModemReadReq()
{
    // Return value. Returns if the flush happen.
    bool _retValue = false;

    // Check if the flush is enabled at all.
    // If not, do nothing!
    if (_flushEspBeforeCmdSend)
    {
        // Now check the state of the handshake pin - if is set to high,
        // ESP32 expects data read - flush needed.
        if (_esp32HandshakePinFlag)
        {
            // Set return value to true since there is data to flush.
            _retValue = true;

            // How many bytes to read.
            uint16_t _len = 0;

            // Keep it flushing until there is no more read requests.
            while (_esp32HandshakePinFlag && requestSlaveStatus(&_len) == INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE)
            {
                // Clear the handshake flag.
                _esp32HandshakePinFlag = false;

                // We "totally read" all data from the ESP32.
                dataRead(_dataBuffer, _len);
                dataReadEnd();

                Serial.println("Flush it!");

                // Wait a little bit for new data.
                delay(5ULL);
            }
        }
    }

    // Return the status of the flush.
    return _retValue;
}

/**
 * @brief   Method waits for the ESP32 module to be ready after power up.
 *
 * @return  bool
 *          true - ESP32 module/modem is ready.
 *          false - ESP32 did not respond with "ready" message.
 * @note    Timeout for the reponse is 5 seconds.
 */
bool WiFiClass::isModemReady()
{
    if (waitForHandshakePinInt(5000ULL))
    {
        // Check for the request, since the Handshake pin is high.
        // Also get the data length.
        uint16_t _dataLen = 0;
        if (requestSlaveStatus(&_dataLen) == INKPLATE_ESP32_SPI_SLAVE_STATUS_READABLE)
        {
            // Ok, now try to read the data. First fill the ESP32 read packet
            dataRead(_dataBuffer, _dataLen);

            // Finish data read.
            dataReadEnd();

            // Parse the data!
            // Add null-terminating char at the end.
            _dataBuffer[_dataLen] = '\0';

            // Compare it. It should find "\r\nready\r\n".
            if (strcmp("\r\nready\r\n", (char *)_dataBuffer) != 0)
            {
                // Serial.println("modem ready parse error!");
                return false;
            }
        }
        else
        {
            // Serial.println("Wrong slave request");
            return false;
        }
    }
    else
    {
        // Serial.println("Handshake not detected");
        return false;
    }

    // Modem ready? Return true for success!
    return true;
}

/**
 * @brief   Method initializes or de-initializes WiFi radio.
 *
 * @param   bool _status
 *          true - Init WiFi radio.
 *          false - Disable WiFi radio.
 * @return  bool
 *          true - Command executed successfully.
 *          false - Command failed.
 */
bool WiFiClass::wiFiModemInit(bool _status)
{
    // Create a AT Commands String depending on the WiFi Initialization status.
    sprintf(_dataBuffer, "AT+CWINIT=%d\r\n", _status);

    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (!sendAtCommandWithResponse(_dataBuffer, 200ULL, 4ULL, (char*)esp32AtCmdResponseOK, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, true, NULL, 0, 0, NULL)) return false;

    // Otherwise return true.
    return true;
}

/**
 * @brief   Helper method for parsing scaned WiFi networks data.
 *
 * @param   int8_t _ssidNumber
 *          Requested scaned nwtwork ID number (internal ID number, not linked to the WiFi network in any way).
 * @param   int8_t *_lastUsedSsidNumber
 *          Pointer tothe variable that holds the last selected ID number.
 * @param   struct spiAtWiFiScanTypedef *_scanData
 *          Can be found in WiFiSPITypedef.h, holds SSID name, auth flag, RSSI value.
 * @return  bool
 *          true - Scaned WiFi network data parsed successfully.
 *          false - Parsing failed.
 */
bool WiFiClass::parseFoundNetworkData(int8_t _ssidNumber, int8_t *_lastUsedSsidNumber,
                                      struct spiAtWiFiScanTypedef *_scanData)
{
    // Check if the last used SSID number matches current one. If so, do not need to parse anything.
    if (*_lastUsedSsidNumber == _ssidNumber)
        return true;

    // If not, check for the SSID number.
    if (_ssidNumber > _foundWiFiAp)
        return false;

    // Try to parse it!
    int _result = sscanf(_dataBuffer + _startApindex[_ssidNumber], "+CWLAP:(%d,\"%[^\",]\",%d", &_scanData->authType,
                         _scanData->ssidName, &_scanData->rssi);

    // Check for parsing. If 3 parameters have been found, parsing is successfull.
    if (_result != 3)
        return false;

    // Otherwise set current one SSID number as last used SSID number.
    *_lastUsedSsidNumber = _ssidNumber;

    // Return true as success.
    return true;
}

/**
 * @brief   Helper method used for parsing IP Addreses )local, gateway or subnet from the ESP32 message.
 *
 * @param   char *_ipAddressType
 *          Select IP data that will be parsed ("ip", "gateway" or "netmask").
 * @return  IPAddress
 *          Selected IP Address with the Arduino IPAddress Class.
 */
IPAddress WiFiClass::ipAddressParse(char *_ipAddressType)
{
    // Array for IP Address. For some reason, STM32 can't parse %hhu so int and %d must be used.
    int _ipAddress[4] = {0, 0, 0, 0};

    // String for filtering IP Adresses from the response.
    char _ipAddressTypeString[30];
    char _ipAddressTypeStringShort[20];
    sprintf(_ipAddressTypeString, "+CIPSTA:%s:\"%%d.%%d.%%d.%%d\"", _ipAddressType);
    sprintf(_ipAddressTypeStringShort, "+CIPSTA:%s:", _ipAddressType);

    // Send AT command and get the response. Check if the response is expected. Otherwise, return false.
    if (sendAtCommandWithResponse((char *)esp32AtWiFiGetIP, 200ULL, 4ULL, _ipAddressTypeStringShort, INKPLATE_ESP32_AT_EXPECTED_RESPONSE_ANY, true, NULL, 0, 0, NULL))
    {
        // Get the exact location of the response.
        char *_ipAddressStart = strstr(_dataBuffer, _ipAddressTypeStringShort);

        // Check one more time. If NULL, no match (no IP address found), return invalid IP address.
        if (_ipAddressStart == NULL) return INADDR_NONE;

        // Get the IP Address from the response.
        int _res =
            sscanf(_ipAddressStart, _ipAddressTypeString, &_ipAddress[0], &_ipAddress[1], &_ipAddress[2], &_ipAddress[3]);

        // If can't find 4 bytes, return invalid IP Address.
        if (_res != 4)
            return INADDR_NONE;
    }

    // Return the IP Address.
    return IPAddress(_ipAddress[0], _ipAddress[1], _ipAddress[2], _ipAddress[3]);
}

// Decalre WiFi class to be globally available and visible.
WiFiClass WiFi;