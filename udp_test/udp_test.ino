#include <InkplateMotion.h>

#define WIFI_SSID ""
#define WIFI_PASS ""
#define REMOTE_IP ""
#define REMOTE_PORT 5005
#define LOCAL_PORT  5005

Inkplate inkplate;
  WiFiUDP udp;

  uint32_t exchanges  = 0;
  uint32_t recoveries = 0;

  void setup()
  {
      Serial.begin(115200);
      inkplate.begin(INKPLATE_1BW);
      WiFi.init();
      
      WiFi.setMode(INKPLATE_WIFI_MODE_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      while (!WiFi.connected()) { Serial.print('.'); delay(1000); }
      delay(3000);
      Serial.print("Inkplate IP: ");
      Serial.println(WiFi.localIP());
      udp.setConnectionTimeout(10000);
      if (!udp.begin(LOCAL_PORT))               { Serial.println("udp.begin FAILED");   while (1); }
      if (!udp.setHost(REMOTE_IP, REMOTE_PORT)) { Serial.println("setHost FAILED");     while (1); }
      if (!udp.beginPacket())                   { Serial.println("beginPacket FAILED"); while (1); }
      Serial.println("Starting stress test...");
  }

  void loop()
  {
      uint32_t txData = exchanges;
      unsigned long t = millis();
      bool ok = udp.write((uint8_t *)&txData, sizeof(txData));
      unsigned long elapsed = millis() - t;

      Serial.print("TX ex="); Serial.print(exchanges);

      if (!ok && elapsed >= 195)
      {
          recoveries++;
          Serial.print("  RECOVERY #"); Serial.print(recoveries);
          Serial.print("  elapsed="); Serial.print(elapsed); Serial.print("ms");
      }
      else if (ok)
      {
          uint32_t rxData = 0;
          udp.read((uint8_t *)&rxData, sizeof(rxData));
          Serial.print("  RX py="); Serial.print(rxData); Serial.print(" (0x"); Serial.print(rxData, HEX); Serial.print(")");
      }

      Serial.println();
      exchanges++;
  }