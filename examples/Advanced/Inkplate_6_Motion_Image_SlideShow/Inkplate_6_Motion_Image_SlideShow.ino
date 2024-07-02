// Include Inkplate Motion Library (STM32).
#include <InkplateMotion.h>

// Include header file iwith all images and their includes.
#include "imageList.h"

// Define pin number for switching the images. Use WakeBtn1 for this purpose.
#define BUTTON_IMAGE_PIN  PG6

// Define pin for enabling screen clear before each image. Use WakeBtn2 for this purpose.
#define BUTTON_SCREEN_CLEAR_FEATURE PA0

// Create Inkplate Motion Library object.
Inkplate inkplate;

// Variables for storing button states for image switching.
bool imageChangeBtnCurrentState = true;
bool imageChangeBntPrevState = true;

// Variable to store feature settings (clear screen after each refresh).
// This feature is set at the start of the code (press and hold wakeBtn2 at start up until screen refresh).
bool aditionalClearBeforeRefresh = false;

// Struct for the image info.
struct imageData
{
    int w;
    int h;
    const uint8_t *imageData = NULL;
};

// Array for easy access of the each image.
// Each row is one image.
struct imageData myImages[] = 
{
  //{animationOneFrame_w, animationOneFrame_h, animationOneFrameArray},
  //{anime1_w, anime1_h, anime1Array},
  //{anime2_w, anime2_h, anime2Array},
  {book_w, book_h, bookArray},
  //{dashboardImage_w, dashboardImage_h, dashboardImageArray},
  //{houses_w, houses_h, housesArray},
  //{lightning1_w, lightning1_h, lightning1Array},
  //{lightning2_w, lightning2_h, lightning2Array},
  {notes_w, notes_h, notesArray},
  //{polygons1_w, polygons1_h, polygons1Array},
  //{solderedLogo_w, solderedLogo_h, solderedLogoArray},
  //{someRandomGuy_w, someRandomGuy_h, someRandomGuyArray},
  {timer_w, timer_h, timerArray},
  {weatherData_w, weatherData_h, weatherDataArray},
  //{weatherMap_w, weatherMap_h, weatherMapArray},
  {welcomeImage_w, welcomeImage_h, welcomeImageArray},
};

// Calculate the number of the images stored in memory.
int numberOfImages = sizeof(myImages) / sizeof(myImages[0]);

// Index variable for accessing individual images from array.
int currentImageIndex = 0;

void setup()
{
    // Enable serial communication.
    Serial.begin(115200);

    // Send debug message on serial.
    Serial.println("Code started!");


    // Initialize pins/buttons.
    pinMode(BUTTON_IMAGE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_SCREEN_CLEAR_FEATURE, INPUT_PULLUP);

    // Check for the screen clear after each refresh feature.
    if (!digitalRead(BUTTON_SCREEN_CLEAR_FEATURE)) aditionalClearBeforeRefresh = true;

    // Initialize Inkplate Motion Library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Clear display.
    inkplate.display();
}

void loop()
{
    waitForTheButtonPress(BUTTON_IMAGE_PIN, &imageChangeBtnCurrentState, &imageChangeBntPrevState);
    updateImage(&inkplate, myImages, numberOfImages, &currentImageIndex, aditionalClearBeforeRefresh);
}

bool checkButtonState(int _pin, bool *_oldState, bool *_newState, bool _triggerState)
{
    // Return value, by default is set to false (no button press detected).
    bool _ret = false;

    // Capture new button/pin state.
    (*_newState) = digitalRead(_pin);

    // Calculate the complementary state of the trigger state.
    bool _compState = !_triggerState;

    // Check for the button press.
    if (((*_oldState) == _compState) && ((*_newState) == _triggerState))
    {
        // Button pressed!
        _ret = true;
    }

    // Update the button/pin state.
    (*_oldState) = (*_newState);

    // Return the button press detection.
    return _ret;
}

void waitForTheButtonPress(int _pin, bool *_oldState, bool *_newState)
{
    // Wait for the button press.
    while (!checkButtonState(_pin, _oldState, _newState, false));

    // Debounce time.
    delay(40);

    // Wait for the button release.
    while (!checkButtonState(_pin, _oldState, _newState, true));
}

void updateImage(Inkplate *_inkplate, struct imageData *_images, int _maxImages, int *_currentImageIndex, bool _additionalRefreshFlag)
{
    // Clear the screen.
    _inkplate->clearDisplay();

    // Draw the image at the center of the screen.
    int _x = (_inkplate->width() - _images[*_currentImageIndex].w) / 2;
    int _y = (_inkplate->height() - _images[*_currentImageIndex].h) / 2;

    // Clear the screen if feature for additional clear is enabled.
    if (_additionalRefreshFlag) _inkplate->display();

    // Draw the image!
    _inkplate->drawBitmap4Bit(_x, _y, _images[*_currentImageIndex].imageData, _images[*_currentImageIndex].w, _images[*_currentImageIndex].h);

    // Refresh the screen.
    _inkplate->display();

    // Increment the index for image access.
    (*_currentImageIndex)++;

    // Check for roll-over.
    if ((*_currentImageIndex) >= _maxImages) (*_currentImageIndex) = 0;
}