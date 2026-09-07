#include <InkplateMotion.h>

#define WIFI_SSID ""
#define WIFI_PASS ""
#define REMOTE_IP ""
#define REMOTE_PORT 5005
#define LOCAL_PORT  5005

Inkplate inkplate;
WiFiUDP udp;

uint32_t attempts  = 0;
uint32_t successes = 0;
uint32_t failures  = 0;
bool     diagnosed = false;

void setup()
{
    Serial.begin(115200);
    inkplate.begin(INKPLATE_1BW);
    WiFi.init();

    WiFi.setMode(INKPLATE_WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.connected()) { Serial.print('.'); delay(1000); }

    IPAddress ip;
    unsigned long ipWaitStart = millis();
    do
    {
        delay(500);
        ip = WiFi.localIP();
    } while (ip == IPAddress(0, 0, 0, 0) && millis() - ipWaitStart < 10000);
    Serial.print("Inkplate IP: ");
    Serial.println(ip);

    udp.setConnectionTimeout(10000);

    Serial.println("Starting beginPacket() stress test (full begin/setHost/beginPacket/end cycle each attempt)...");
}

void loop()
{
    attempts++;

    bool beginOk = udp.begin(LOCAL_PORT);
    bool hostOk  = beginOk && udp.setHost(REMOTE_IP, REMOTE_PORT);
    bool ok      = hostOk && udp.beginPacket();

    if (ok)
        successes++;
    else
        failures++;

    // Dump whatever raw text the ESP32 last sent back, once, right when the failure pattern
    // first appears - so we know if it's an AT "ERROR", a timeout (stale/empty buffer), or
    // something else, instead of just true/false.
    if (!ok && !diagnosed)
    {
        diagnosed = true;
        Serial.println("=== FIRST FAILURE - raw AT buffer follows ===");
        Serial.print("begin failed: ");    Serial.println(!beginOk);
        Serial.print("setHost failed: ");  Serial.println(beginOk && !hostOk);
        Serial.print("beginPacket failed: "); Serial.println(hostOk && !ok);
        Serial.println(WiFi.getDataBuffer());
        Serial.println("=== end raw AT buffer ===");
    }

    Serial.print("attempt="); Serial.print(attempts);
    Serial.print(" begin="); Serial.print(beginOk ? "OK" : "FAIL");
    Serial.print(" setHost="); Serial.print(hostOk ? "OK" : "FAIL");
    Serial.print(" beginPacket="); Serial.print(ok ? "OK" : "FAIL");
    Serial.print(" successes="); Serial.print(successes);
    Serial.print(" failures="); Serial.print(failures);
    Serial.print(" rate="); Serial.print((100.0 * successes) / attempts, 1);
    Serial.println("%");

    udp.end();
    delay(200);
}
