// Include Inkplate Motion library.
#include <InkplateMotion.h>

// Create Inkplate object.
Inkplate inkplate;

void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Hello from Inkplate 6 Motion!");

    // Initialize Inkplate Motion library.
    inkplate.begin();

    // Set text size to 3.
    inkplate.setTextSize(3);

    // Force full update every 40 partial updates.
    inkplate.setFullUpdateTreshold(40);

    // Initialize APDS9960.
    if (!inkplate.apds9960.init())
    {
        // If init failed print out debug message.
        inkplate.println("APDS9960 Init failed!");

        // Stop the code!
        while (1);
    }
  
    // Start the APDS-9960 proximity sensor with no interrupts enabled.
    if (!inkplate.apds9960.enableProximitySensor(false))
    {
        // Print out debug message if failed.
        inkplate.println("Proximity sensor enable failed!");

        // Stop the code!
        while (1);
    }

    // Enable color sensor without interrupts.
    if (!inkplate.apds9960.enableLightSensor(false))
    {
        // Print out debug message in the case of sensor enable fail.
        inkplate.println("Light sensor enable failed!");

        // Stop the code!
        while (1);
    }

    // Wait for initialization and calibration to finish
    delay(500);
}

// Variable for timing (to read RGB values and proximity every second without blocking delay).
unsigned long readIntervalTimer = 0;

void loop()
{
    // Read proximity, ALS and color every 250ms using non-blocking method.
    if ((unsigned long)(millis() - readIntervalTimer) > 250ULL)
    {
        // Capture new timestamp.
        readIntervalTimer = millis();

        // Local variable used to store measuerd data.
        uint16_t ambientLight = 0;
        uint16_t redChannel = 0;
        uint16_t greenChannel = 0;
        uint16_t blueChannel = 0;
        uint8_t proximity = 0;

        // Read the light levels (ambient, red, green, blue).
        if (inkplate.apds9960.readAmbientLight(ambientLight) &&
            inkplate.apds9960.readRedLight(redChannel) &&
            inkplate.apds9960.readGreenLight(greenChannel) &&
            inkplate.apds9960.readBlueLight(blueChannel))

        {
            // If read was successful, print RGB channels and ALS.
            drawBarGraph(&inkplate, 300, 150, 500, 50, redChannel, 37889, BLACK, "Red Channel");
            drawBarGraph(&inkplate, 300, 250, 500, 50, greenChannel, 37889, BLACK, "Green Channel");
            drawBarGraph(&inkplate, 300, 350, 500, 50, blueChannel, 37889, BLACK, "Blue Channel");
            drawBarGraph(&inkplate, 300, 450, 500, 50, ambientLight, 37889, BLACK, "Ambient light");
            inkplate.partialUpdate();
        }

        // Try to read proximity value.
        if (inkplate.apds9960.readProximity(proximity))
        {
            // If read was successful, print out the value.
            drawBarGraph(&inkplate, 300, 550, 500, 50, proximity, 255, BLACK, "Proximity");
        }
    }

    // Do something else here...
}

// Function to draw the bar graph, print raw values, and label the graphs.
void drawBarGraph(Inkplate *_inkplate, int x, int y, int width, int height, int value, int maxValue, uint16_t color, const char* label)
{
    // Draw the label
    _inkplate->setCursor(x - 250, y + (height / 4)); // Adjust for vertical centering
    _inkplate->print(label);
    
    // Clear previous bar graph area
    _inkplate->fillRect(x - 1, y - 1, width + 2, height + 2, WHITE); // Add some padding to ensure complete clearing
    
    // Draw the bar graph outline and filled bar
    int barWidth = map(value, 0, maxValue, 0, width - 10); // 5 pixels space on each side
    _inkplate->drawRoundRect(x, y, width, height, 5, BLACK);
    _inkplate->fillRoundRect(x + 5, y + 5, barWidth, height - 10, 5, color);

    // Clear previous value
    _inkplate->fillRect(x + width + 10, y, 150, height, WHITE);
    
    // Print raw value
    _inkplate->setCursor(x + width + 10, y + (height / 4)); // Adjust for vertical centering
    _inkplate->print(value);
}