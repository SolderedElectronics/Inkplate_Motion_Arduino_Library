// Add headerguard do prevent multiple include.
#ifndef __ESP32_SPI_AT_MQTT_H__
#define __ESP32_SPI_AT_MQTT_H__

// Include main Arduino header file.
#include <Arduino.h>

// Include main ESP32-C3 AT SPI library.
#include "esp32SpiAt.h"

// Class for MQTT over SPI AT commands.
class WiFiMQTT
{
  public:
    // Constructor for thw MQTT class.
    WiFiMQTT();

    // Destructor for the MQTT class - needed because of memory allocation.
    ~WiFiMQTT();

    // Initializers for the library.
    bool begin(uint16_t _rxBufferSize = 1024);
    bool begin(uint8_t *_rxBuffer, uint16_t _rxBufferSize);

    // Set the used MQTT server - only one server at the time, no matter how many different MQTT objects are used!
    bool setServer(char *_mqttServer, uint16_t port);

    // Try to connect to the MQTT server/broker. If no username, clientID and password are used, use empty string (""),
    // DO NOT use NULL!
    bool connect(char *_clientId = NULL, char *_userName = NULL, char *_password = NULL);

    // Subscribe to the MQTT topic (you can subscribe to the multiple topics).
    bool subscribe(const char *_topic);

    // If data is received, this function will return topic name.
    char* topic();

    // Disconnect from the server/broker.
    bool disconnect();

    // Un-sub from the specified topic.
    bool unsubscribe(char *_topic);

    // Publish data to specified topic.
    bool publish(char *_topic, char *_payload, uint16_t _len = 0, bool _retain = 0);
    
    // Check if there is connection to the MQTT broker.
    bool connected();

    // Set if reconnection are allowed. This must be set before connect()!
    void reconnect(uint8_t _reconnectMode);

    // Set QoS. Must be set before publish and subscribe!
    void setQoS(int _qos);

    // Read the data recevied by ESP32 from the MQTT.
    uint16_t read(uint8_t *_buffer, uint16_t _len);
    char read();

    // Returns how many data have been received.
    uint16_t available();

    // This needs to be in the loop to check incomming data. This method is responable for the data receive.
    void loop();

  private:
    void parseMQTTData(char *_cmdHeader);

    // Buffer for storing last received topic. Limited to the first 256 chars.
    char _lastRxTopic[256];

    // Pointer to the main AT command buffer from the WiFi library itself. Memory reusage!
    // But there must be a better way to handle this.
    char *_atCommandBuffer = NULL;

    // Pointer to the RX data buffer for subscribed topics. It can be user-defined or allocated.
    char *_rxDataBuffer = NULL;

    // Flag if the RX buffer for MQTT subscribed topic data is allocated (set to true) or user-defined buffer is used (false).
    bool _allocated = false;

    // Pointer pointed to the current last read data byte in the RX buffer, used by the read method.
    char *_currentPosition = NULL;

    // RX buffer size (user-defined or allocated).
    uint16_t _maxRxBufferSize = 0;

    // Pointer to the server name address.
    char *_serverPtr = NULL;

    // MQTT server port.
    uint16_t _port = 0;

    // Automatic reconnect mode (0 if not used). If automatic reconnect is used, everything is handled by the ESP32.
    uint8_t _reconnect = 0;

    // QoS MQTT parameter.
    uint8_t _QoSParameter = 0;

    // Variable holds number of received data ready to be used by the read() method.
    uint16_t _dataAvailable = 0;
};

#endif