// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create Inkplate Motion object.
Inkplate inkplate;

// Partial update counter variable.
uint8_t partialUpdateCounter = 0;

void setup()
{
    // Initialize Serial Communication at 115200 bauds.
    Serial.begin(115200);

    // Print debug message.
    Serial.println("Hello from Inkplate 6 Motion");

    // Initialize Inkplate Motion Libary.
    inkplate.begin();

    // Initialize SHTC3 sensor.
    inkplate.shtc3.begin();

    // Set text size.
    inkplate.setTextSize(4);

    // Set text color to black and background color to white.
    inkplate.setTextColor(BLACK, WHITE);

    // Set automatic full update after 30 partial updates.
    inkplate.setFullUpdateTreshold(0);
}

void loop()
{
    // Force SHTC3 sensor update. If update was successful, update the screen with new data.
    if (inkplate.shtc3.update() == SHTC3_Status_Nominal)
    {
        // Set print cursor position.
        inkplate.setCursor(100, 350);

        inkplate.print("Temp: ");
        inkplate.print(inkplate.shtc3.toDegC(), 2);
        inkplate.println("*C");

        // Set print cursor position.
        inkplate.setCursor(100, inkplate.getCursorY());
        inkplate.print("Hum: ");
        inkplate.print(inkplate.shtc3.toPercent(), 2);
        inkplate.print('%');

        // Update the screen. If more than 30 partial updates are done, full update will be executed.
        inkplate.partialUpdate(true);
    }

    // Update every 2 seconds.
    delay(2000);
}