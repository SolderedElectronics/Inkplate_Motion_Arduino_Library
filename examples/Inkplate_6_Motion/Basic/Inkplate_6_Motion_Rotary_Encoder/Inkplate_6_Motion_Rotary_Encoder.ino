// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Create Inkplate Motion Arduino Library object.
Inkplate inkplate;

// Variables holds current and previous angle of the rotary encoder.
int currentValue = 0;
int oldValue = 0;

// Fast and easy conversion from degrees to radians.
const float DEG2RAD = PI / 180.0f;

void setup()
{
    // Initialize Inkplate Motion library.
    inkplate.begin();

    // Enable power to the rotary encoder.
    inkplate.peripheral(INKPLATE_ROTARY_ENCODER_PERIPH, true);
    // Initialize rotery encoder.
    inkplate.rotaryEncoder.begin();

    // Enble power to the WS addressable LED.
    inkplate.peripheral(INKPLATE_WS_LED_PERIPH, true);
    // Initialize WS addressable LED library.
    inkplate.led.begin();
    // Set LED color to purple.
    inkplate.led.setPixelColor(INKPLATE_WSLED_ROTARY_ENCODER, 64, 0, 128);
    inkplate.led.show();

    // Refresh the display (clear everything from the screen).
    inkplate.display();

    // Draw circle and line on the screen. Line can be rotated by moving rotary encoder.
    printPosiiton(oldValue, currentValue);
    inkplate.display(true);
}


void loop()
{
    // Get the current position of the rotary encoder.
    currentValue = (int)(inkplate.rotaryEncoder.rawAngle() * AS5600_RAW_TO_DEGREES);

    // Treshold is 2 degrees to update the position of the line on the screen.
    if (abs(currentValue - oldValue) >= 2)
    {
        // Update the position of the line.
        printPosiiton(oldValue, currentValue);

        // Update the screen (keep the epaper power supply active all
        // the time - faster screen refresh!).
        inkplate.partialUpdate(true);

        // Update the old value.
        oldValue = currentValue;
    }
}

void printPosiiton(int _oldPosition, int _newPos)
{
    // Clear prev. data.
    int _x = (300 * cos(DEG2RAD * _oldPosition)) + (inkplate.width() / 2);
    int _y = (300 * sin(DEG2RAD * _oldPosition)) + (inkplate.height() / 2);
    inkplate.drawLine(inkplate.width() / 2, inkplate.height() / 2, _x, _y, WHITE);

    // Redraw the circle.
    inkplate.drawCircle(inkplate.width() / 2, inkplate.height() / 2, 310, BLACK);

    // Draw new posiiton.
    _x = (300 * cos(DEG2RAD * _newPos)) + (inkplate.width() / 2);
    _y = (300 * sin(DEG2RAD * _newPos)) + (inkplate.height() / 2);
    inkplate.drawLine(inkplate.width() / 2, inkplate.height() / 2, _x, _y, BLACK);
}
