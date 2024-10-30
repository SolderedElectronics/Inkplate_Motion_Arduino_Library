// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Change WiFi SSID and password here.
#define WIFI_SSID "Soldered"
#define WIFI_PASS "dasduino"


// Select one of the HTTP links.
char httpUrl[] = {"https://filesampleshub.com/download/image/bmp/sample1.bmp"};
// char httpUrl[] = {"https://raw.githubusercontent.com/BornaBiro/ESP32-C3-SPI-AT-Commands/main/lorem_ipsum_long.txt"};
// char httpUrl[] = {"https://raw.githubusercontent.com/BornaBiro/ESP32-C3-SPI-AT-Commands/main/lorem_ipsum.txt"};

// Create an Inkplate Motion Object.
Inkplate inkplate;

void setup() {
  // Setup a Serial communication for debug at 115200 bauds.
  Serial.begin(115200);

  // Print an welcome message (to know if the Inkplate board and STM32 are alive).
  Serial.println("Inkplate Motion Code Started!");

  // Initialize the Inkplate Motion Library.
  inkplate.begin(INKPLATE_GL16);

  // Clear the screen.
  inkplate.display();

  // Set the text.
  inkplate.setCursor(0, 0);
  inkplate.setTextSize(2);
  inkplate.setTextColor(BLACK, WHITE);
  inkplate.setTextWrap(true);

  // Initialize ESP32 AT Commands Over SPI library.
  if (!WiFi.init()) {
    inkplate.println("ESP32-C3 initializaiton Failed! Code stopped.");
    inkplate.partialUpdate(true);

    while (1) {
      delay(100);
    }
  }
  inkplate.println("ESP32 Initialization OK!");
  inkplate.partialUpdate(true);

  // Set it back to the station mode.
  if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA)) {
    inkplate.println("STA mode failed!");
    inkplate.partialUpdate(true);

    while (1) {
      delay(100);
    }
  }

  // Connect to the WiFi network.
  inkplate.print("Connecting to ");
  inkplate.print(WIFI_SSID);
  inkplate.print(" wifi...");
  inkplate.partialUpdate(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (!WiFi.connected()) {
    inkplate.print('.');
    inkplate.partialUpdate(true);
    delay(1000);
  }
  inkplate.println("connected!");
  inkplate.partialUpdate(true);

  Serial.println("drum roll mf");
  Serial.println(inkplate.image.draw(httpUrl, 0, 0));
  
  Serial.println("end");
  // Update the Inkplate display to show the printed messages
  inkplate.display();
}

void loop() {
}
