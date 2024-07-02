// Add a header guard.
#ifndef __ESP32_SPI_AT_ALL_COMMANDS_H__
#define __ESP32_SPI_AT_ALL_COMMANDS_H__

// ESP32 System AT Commands
static const char esp32AtPingCommand[] = "AT\r\n";
static const char esp32AtPingResponse[] = "AT\r\n\r\nOK\r\n";
static const char esp32AtCmdResponse[] = "\r\n\r\nOK\r\n";
static const char esp32AtCmdResponseOK[] = "\r\nOK\r\n";
static const char esp32AtCmdResponseError[] = "\r\n\r\nERROR\r\n";
static const char esp32AtCmdSystemRestore[] = "AT+RESTORE\r\n";
static const char esp32AtCmdEscapeChar[] = {0x1B, 0x00};

// ESP32 WiFi Commands
// ESP32 AT Command to disconnect from the AP.
static const char esp32AtWiFiDisconnectCommand[] = "AT+CWQAP\r\n";
// ESP32 Response on Disconnect From AP Command.
static const char esp32AtWiFiDisconnectresponse[] = "AT+CWQAP\r\n\r\nOK\r\n";
// Start WiFi Scan AT Command.
static const char esp32AtWiFiScan[] = "AT+CWLAP\r\n";
// Get IP address of the ESP32-C3 Station.
static const char esp32AtWiFiGetIP[] = "AT+CIPSTA?\r\n";
// Get ESP32 MAC Address.
static const char esp32AtWiFiGetMac[] = "AT+CIPAPMAC?\r\n";

// TCP/TP AT Commands.
static const char esp32AtGetDns[] = "AT+CIPDNS?\r\n";
#endif