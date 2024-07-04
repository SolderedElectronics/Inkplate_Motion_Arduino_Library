// Include Inkplate Motion libary.
#include "InkplateMotion.h"

// Include Adafruit GFX Fonts.
#include "FreeSansBold24pt7b.h"

// Include the file containing bitmap of each gesture icon.
#include "icons.h"

// Initialize Inkplate
Inkplate inkplate;

// ISR flag - Automatically set to true in case of Interrupt event from the IO Expander.
volatile bool isrFlag = false;

// ISR handler function - Called when IO expander fires INT.
void ioExpanderISR()
{
    isrFlag = true;
}

void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Hello from Inkplate 6 Motion!");
    
    // Initialize Inkplate
    inkplate.begin();

    // Set APDS INT pin on IO Expander as input. Override any GPIO pin protection.
    inkplate.internalIO.pinModeIO(IO_PIN_A0, INPUT, true);

    // Set interrupts on IO expander.
    inkplate.internalIO.setIntPinIO(IO_PIN_A0);

    // Enable interrptus on STM32.
    // NOTE: Must be set to change!
    attachInterrupt(digitalPinToInterrupt(PG13), ioExpanderISR, CHANGE);

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
    if (!inkplate.apds9960.enableGestureSensor(true))
    {
        // Print error message if failed.
        Serial.println("Gesture sensor failed to start");

        // Stop the code!
        while (1);
    }

    // Wait a little bit.
    delay(250);

    // Set gesture sensitivity level (higher the number = higher sensitivity).
    inkplate.apds9960.setGestureGain(0);

    // Clear out any previous detected gestures.
    checkGesture(NULL);
    
    // Draw the static GUI elements.
    drawGUI();
}

void loop()
{
    // Check if Interrupt from the IO expander has occured.
    if (isrFlag)
    {
        // Send debug messsage on UART/Serial.
        Serial.println("Interrupt from APDS9960 detected.");

        // Check if the INT pin for the APDS9960 is set to low. Otherwise ignore the INT event
        // (must be set to low for INT event from APDS). Override any GPIO pin protection.
        if (!inkplate.internalIO.digitalReadIO(IO_PIN_A0, true))
        {
            // If there is some interrupt event, check if this interrupt event is
            // gesture eveent at all. And save the gesture into variable.
            uint8_t detectedGesture = 0;
            if (checkGesture(&detectedGesture))
            {
                // Print the gesture on the screen.
                switch (detectedGesture)
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
        // Clear the flag so that new interrupt event can be detected.
        isrFlag = false;
    }
}

bool checkGesture(uint8_t *_gesturePtr)
{
    // Check if there is any gesture detected at all.
    if (inkplate.apds9960.isGestureAvailable())
    {
        // If it is, read it.
        // NOTE: If the gesture is detected, but it's not read, sensor won't fire any interrupt
        // until the gesture is read from the register.
        uint8_t _gesture = inkplate.apds9960.readGesture();

        // If the pointer for detected gesture is not NULL, copy the result.
        if (_gesturePtr != NULL) (*_gesturePtr) = _gesture;

        // Return true for successfully detected gesture.
        return true;
    }
    
    // Otherwise return false.
    return false;
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