// Include Inkplate Motion libary.
#include "InkplateMotion.h"

// Include Adafruit GFX Fonts.
#include "FreeSansBold24pt7b.h"

// Include the file containing bitmap of each gesture icon.
#include "icons.h"

// Initialize Inkplate
Inkplate inkplate;

void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Hello from Inkplate 6 Motion!");
    
    // Initialize Inkplate
    inkplate.begin();

    // Use default 5x7 font scaled 3 times.
    inkplate.setTextSize(3);
    
    // Initialize APDS9960. Notify user if init has failed.
    if (!inkplate.apds9960.init())
    {
        inkplate.println("APDS-9960 initialization failed");

        // Stop the code!
        while (1);
    }
    
    // Start running the APDS9960 gesture sensor engine.
    if (!inkplate.apds9960.enableGestureSensor(false))
    {
        // Print error message if failed.
        Serial.println("Gesture sensor failed to start");

        // Stop the code!
        while (1);
    }
    
    // Draw the static GUI elements.
    drawGUI();
}

void loop()
{
    // Check if new gesture is detected.
    if (inkplate.apds9960.isGestureAvailable())
    {
        // Get the detected gesture and print the gesture on the screen.
        switch (inkplate.apds9960.readGesture())
        {
          case DIR_UP:
            updateGestureDisplay("Up", gestureUp);
            break;
          case DIR_DOWN:
            updateGestureDisplay("Down", gestureDown);
            break;
          case DIR_LEFT:
            updateGestureDisplay("Left", gestureLeft);
            break;
          case DIR_RIGHT:
            updateGestureDisplay("Right", gestureRight);
            break;
          case DIR_NEAR:
            updateGestureDisplay("Near", gestureNear);
            break;
          case DIR_FAR:
            updateGestureDisplay("Far", gestureFar);
            break;
          default:
            // If no gesture is detected, but something is detected, print out "?".
            updateGestureDisplay("Huh?", gestureNone);
        }
    }
}

// Draw static text elements.
void drawGUI()
{
    // Variables for text aligment.
    int16_t  _x1, _y1;
    uint16_t _w, _h;

    // Strings (title and text below detected desture image).
    char _title[] = {"Wave with your hand over the sensor"};
    char _detectedGEstureString[] = {"Detected gesture"};
    
    // Font settings. Use default scaling and use Adafruit GFX provided font. Set text color to black.
    inkplate.setTextSize(1);
    inkplate.setTextColor(BLACK);
    inkplate.setFont(&FreeSansBold24pt7b);

    // Draw the title and also center it!
    inkplate.getTextBounds(_title, 0, 0, &_x1, &_y1, &_w, &_h);
    inkplate.setCursor((1024 - _w) / 2, 30);
    inkplate.print(_title);
    inkplate.drawRect(310, 162, 404, 404, BLACK);
    
    // Draw the text below detected gesture and center it!
    inkplate.getTextBounds(_detectedGEstureString, 0, 0, &_x1, &_y1, &_w, &_h);
    inkplate.setCursor((1024 - _w) / 2, 600); // Center the "Detected gesture" string
    inkplate.print(_detectedGEstureString);
    
    // Send everything to the screen.
    inkplate.partialUpdate();
}

// Function draws detected gesture icon and also gesture name,
void updateGestureDisplay(const char* _gesture, const uint8_t* _bitmap)
{
    // Variables for text aligment.
    int16_t  _x1, _y1;
    uint16_t _w, _h;
    
    // Clear previous image.
    inkplate.fillRect(312, 164, 400, 400, WHITE);

    // Check if the bitmap image/icon is not nullptr.
    if (_bitmap)
    {
        // Draw new bitmap according to the detected gesture.
        inkplate.drawBitmap(312, 164, _bitmap, 400, 400, BLACK);
    }
    
    // Clear previous gesture text.
    inkplate.fillRect(312, 660, 400, 80, WHITE);
    
    // Adjust the y position and height to draw text of detected gesture in the center.
    inkplate.getTextBounds(_gesture, 0, 0, &_x1, &_y1, &_w, &_h);
    inkplate.setCursor((1024 - _w) / 2, 720);
    inkplate.print(_gesture);
    
    // Send everything to the screen.
    inkplate.partialUpdate();
}