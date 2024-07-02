// Add headerguard do prevent multiple include.
#ifndef __ESP32_SPI_AT_UDP_H__
#define __ESP32_SPI_AT_UDP_H__

// Include main Arduino header file.
#include <Arduino.h>

// Include main ESP32-C3 AT SPI library.
#include "esp32SpiAt.h"

// Class for the ESP32 SPI UDP.
class WiFiUDP
{
  public:
    // Class constructor.
    WiFiUDP();

    // Initializer for the UDP.
    bool begin(uint16_t _localPort);

    // Sets the host by name or by IP address.
    bool setHost(const char *_host, uint16_t _hostPort);

    // Initializer for packet transfer.
    bool beginPacket();

    // Send data to the desired web server to it's destination port.
    bool write(uint8_t *_packet, uint16_t _len);

    // Returns how many bytes are available for read.
    uint16_t available(bool _blocking = false);

    // read bytes from the RX buffer.
    int read(uint8_t *_data, uint16_t _len);

    // End UDP connection. Also remove all filters for data transfer.
    bool end();

    // Sets connection timeout in milliseconds.
    void setConnectionTimeout(uint16_t _connectionTimeout);

  private:
    uint16_t _localUdpPort = 0;
    uint16_t _availableData = 0;
    char *_currentPosition = 0;
    uint16_t _connectionTimeoutValue = 20000ULL;
    char *_dataBuffer = NULL;
};

#endif