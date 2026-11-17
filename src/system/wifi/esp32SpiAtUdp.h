/**
 **************************************************
 *
 * @file        esp32SpiAtUdp.cpp
 * @brief       Header file for the UDP communicaiton with the ESP32.
 *              This file is used with esp32SpiAt library.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

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
    ~WiFiUDP();

    // Owns a heap RX buffer, so copying would double-free it.
    WiFiUDP(const WiFiUDP &) = delete;
    WiFiUDP &operator=(const WiFiUDP &) = delete;

    // Initializer for the UDP.
    bool begin(uint16_t _localPort);

    // Sets the host by name or by IP address.
    bool setHost(const char *_host, uint16_t _hostPort);

    // Initializer for packet transfer.
    bool beginPacket();

    // Send data to the desired web server to it's destination port.
    bool write(uint8_t *_packet, uint16_t _len);

    // Returns how many bytes are available for read.
    // Note: a packet that arrives while another WiFi.* AT command is running is flushed by the
    // ESP32 driver and lost. Data already returned by available() is safe until read().
    uint16_t available(bool _blocking = false);

    // read bytes from the RX buffer.
    int read(uint8_t *_data, uint16_t _len);

    // End UDP connection. Also remove all filters for data transfer.
    bool end();

    // Sets connection timeout in milliseconds.
    void setConnectionTimeout(uint16_t _connectionTimeout);

  private:
    void keepRxData(char *_data, uint16_t _len);

    uint16_t _localUdpPort = 0;
    uint16_t _availableData = 0;
    char *_currentPosition = 0;
    uint16_t _connectionTimeoutValue = 20000ULL;
    char *_dataBuffer = NULL;

    // Private RX buffer. Received data must not live in WiFi's shared AT buffer, otherwise any
    // WiFi.* AT call (e.g. currentRSSI()) between available() and read() overwrites it.
    char *_rxBuffer = NULL;
};

#endif