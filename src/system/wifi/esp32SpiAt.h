// Add headerguard do prevent multiple include.
#ifndef __ESP32_SPI_AT_H__
#define __ESP32_SPI_AT_H__

// Add main Arduino header file.
#include <Arduino.h>

// include Arduino Library for the IP Adresses.
#include <IPAddress.h>

// Include Arduino SPI library.
#include <SPI.h>

// Include SPI AT Message typedefs.
#include "WiFiSPITypedef.h"

// Include file with all AT Commands.
#include "esp32SpiAtAllCommands.h"

// Include HTTP class for ESP32 AT Commands.
#include "esp32SpiAtHttp.h"

// Include UDP class for ESP32 AT Commands.
#include "esp32SpiAtUdp.h"

// Include MQTT class for ESP32 AT Commands.
#include "esp32SpiAtMqtt.h"

// Data buffer for AT Commands (in bytes).
#define INKPLATE_ESP32_AT_CMD_BUFFER_SIZE 8192ULL

// GPIO pin for the ESP32 Power Supply Switch.
#define INKPLATE_ESP32_PWR_SWITCH_PIN PG9

// GPIO Pins for the SPI communication with the ESP32.
#define INKPLATE_ESP32_MISO_PIN      PF8
#define INKPLATE_ESP32_MOSI_PIN      PF9
#define INKPLATE_ESP32_SCK_PIN       PF7
#define INKPLATE_ESP32_CS_PIN        PF6
#define INKPLATE_ESP32_HANDSHAKE_PIN PA15

// Maximum networks that can be found.
#define INKPLATE_ESP32_MAX_SCAN_AP  40

// Create class for the AT commands over SPI

class WiFiClass
{
  public:
    WiFiClass();

    // Public ESP32-C3 system functions.
    void hwSetup(SPIClass *_spiClass);
    bool init(bool _resetSettings = true);
    bool power(bool _en, bool _resetSettings = true);
    bool sendAtCommand(char *_atCommand, uint16_t _len = 0);
    bool getAtResponse(char *_response, uint32_t _bufferLen, unsigned long _timeout, uint16_t *_rxLen = NULL);
    bool getSimpleAtResponse(char *_response, uint32_t _bufferLen, unsigned long _timeout, uint16_t *_rxLen = NULL);
    bool sendAtCommandWithResponse(char *_atCommand = NULL, unsigned long _timeoutAtCommand = 0ULL, unsigned long _rxDataTimeoutAtCommand = 0ULL, char *_expectedResponseAtCmd = NULL, uint8_t _expectedResponsePosition = INKPLATE_ESP32_AT_EXPECTED_RESPONSE_START, bool _terminateOnAtResponseError = true, char *_dataPart = NULL, uint16_t _dataPartLen = 0, unsigned long _timeoutDataPart = 0ULL, char *_expectedResponseData = NULL, uint16_t *_len = NULL);
    bool sendDataPart(char *_atCommand, char *_dataPart, uint16_t _dataPartLen, unsigned long _timeout, char *_expectedResponse);
    bool messageFilter(bool _enable, char* _headFilter, char *_tailFilter);
    bool commandEcho(bool _en);
    bool modemPing();
    bool systemRestore();
    bool storeSettingsInNVM(bool _store);
    char *getDataBuffer();
    bool systemMessages(uint8_t _cfg);
    void flushBeforeCommand(bool _en);
    bool defaultMsgFiltersEn();
    bool systemMsgFiltering(bool _en);

    // Public ESP32 WiFi Functions.
    bool setMode(uint8_t _wifiMode);
    bool begin(char *_ssid, char *_pass);
    bool connected();
    bool disconnect();
    int scanNetworks();
    char *ssid(int _ssidNumber);
    bool auth(int _ssidNumber);
    int rssi(int _ssidNumber);
    IPAddress localIP();
    IPAddress gatewayIP();
    IPAddress subnetMask();
    IPAddress dns(uint8_t i);
    char *macAddress();
    bool macAddress(char *_mac);
    bool config(IPAddress _staticIP = INADDR_NONE, IPAddress _gateway = INADDR_NONE, IPAddress _subnet = INADDR_NONE,
                IPAddress _dns1 = INADDR_NONE, IPAddress _dns2 = INADDR_NONE);
    bool waitForHandshakePin(uint32_t _timeoutValue, bool _validState = HIGH);
    bool getHandshakePinState();
  private:
    // ESP32 SPI Communication Protocol methods.
    bool waitForHandshakePinInt(uint32_t _timeoutValue);
    uint8_t requestSlaveStatus(uint16_t *_len = NULL);
    bool dataSend(char *_dataBuffer, uint32_t _len);
    bool dataSendEnd();
    bool dataRead(char *_dataBuffer, uint16_t _len);
    bool dataReadEnd();
    bool dataSendRequest(uint16_t _len, uint8_t _seqNumber);
    void transferSpiPacket(spiAtCommandTypedef *_spiPacket, uint16_t _spiPacketLen);
    void sendSpiPacket(spiAtCommandTypedef *_spiPacket, uint16_t _spiDataLen);
    // End of ESP32 SPI Communication Protocol methods.

    // Modem related methods.
    bool flushModemReadReq();
    bool isModemReady();
    bool wiFiModemInit(bool _status);
    bool parseFoundNetworkData(int8_t _ssidNumber, int8_t *_lastUsedSsidNumber, struct spiAtWiFiScanTypedef *_scanData);
    IPAddress ipAddressParse(char *_ipAddressType);

    // Data buffer for the ESP32 SPI commands.
    char _dataBuffer[INKPLATE_ESP32_AT_CMD_BUFFER_SIZE];

    // SPI Class pointer.
    SPIClass *_spi;

    // Variables for WiFi Scan.
    int16_t _startApindex[INKPLATE_ESP32_MAX_SCAN_AP];
    uint8_t _foundWiFiAp = 0;
    int8_t _lastUsedSsid = -1;
    struct spiAtWiFiScanTypedef _lastUsedSsidData;
    char _invalidMac[18] = {"00:00:00:00:00:00"};
    // Array for storing parsed MAC address.
    char _esp32MacAddress[19];

    // Flag used for indicating that WiFi connection to the AP is currently in progress.
    bool _wifiConnectionInProgres = false;

    // Flag for enabling/disabling flushing ESP32 from all read requests before AT Command send.
    bool _flushEspBeforeCmdSend = true;
};

// For easier user usage of the WiFi functionallity.
extern WiFiClass WiFi;

#endif