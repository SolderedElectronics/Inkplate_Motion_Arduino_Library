/**
 **************************************************
 * @file        Inkplate_6_Motion_Factory_Programming.ino
 *
 * @brief       File for initial testing and displaying the onboarding seqeuence of Inkplate 6 MOTION
 *
 * @note        This sketch will test part of the features which need to be tested when Inkplate 6 MOTION
 * is assembled and in it's enclosure.
 *
 *              To pass all tests:
 *              - Edit the WiFi information below
 *              - Follow all the test's instructions
 *
 *              The other tests are done in Inkplate_6_Motion_VCOM_Set.ino
 *
 *              After all tests have passed the device will showcase the Inkplate 6MOTION onboarding image sequence
 *
 *License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html Please review the
 *LICENSE file included with this example. If you have any questions about
 *licensing, please visit https://soldered.com/contact/ Distributed as-is; no
 *warranty is given.
 *
 * @authors     Robert @ Soldered
 ***************************************************/

// Test parameters which may be changed if required:

// If you want to test the device again change this
// Can't be 0 as this is the default EEPROMoffset for VCOM writing
int testsDoneEepromValue = 30; // Change this number if you want to repeat tests
int testsDoneEepromOffset = 8; // Where in EEPROM this symbollic value is written

// WiFi credentials for testing
char *wifiSSID = {"Soldered-testingPurposes"};
char *wifiPASS = {"Testing443"};

// The qwiic (I2C) follower address which will be checked
const uint8_t qwiicTestAddress = 0x30;

// More detailed test parameters are available to edit in InkplateTest.cpp

////////////////////////////////////////////////////////////////////////////////////////////////////

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(BOARD_INKPLATE6_MOTION)
#error "Wrong board selection for this example, please select Inkplate 6MOTION in the boards menu."
#endif

// Include Inkplate Motion library for STM32H743 MCU
#include <InkplateMotion.h>

// Include Inkplate test class header file
#include "InkplateTest.h"

// Include slideshow slides
#include "slides/onboarding_00_min.h"
#include "slides/onboarding_01_min.h"
#include "slides/onboarding_02_min.h"
#include "slides/onboarding_03_min.h"
#include "slides/onboarding_04_min.h"
#include "slides/onboarding_05_min.h"
#include "slides/onboarding_06_min.h"
#include "slides/onboarding_07_min.h"
#include "slides/onboarding_initial_min.h"
#include "slides/solderedLogoSmall.h"

// Sketch varaibles
Inkplate inkplate;      // Create an Inkplate Motion object.
InkplateTest testClass; // The class which does all the testing

// This is the onboarding image sequence
// Declare an array of pointers to the arrays
void *onboardingSlides[9] = {
    (void *)onboarding_initial_min, (void *)onboarding_00_min, (void *)onboarding_01_min,
    (void *)onboarding_02_min,      (void *)onboarding_03_min, (void *)onboarding_04_min,
    (void *)onboarding_05_min,      (void *)onboarding_06_min, (void *)onboarding_07_min,
};
// Also know the sizes
size_t slideSizes[9] = {
    sizeof(onboarding_initial_min), sizeof(onboarding_00_min), sizeof(onboarding_01_min),
    sizeof(onboarding_02_min),      sizeof(onboarding_03_min), sizeof(onboarding_04_min),
    sizeof(onboarding_05_min),      sizeof(onboarding_06_min), sizeof(onboarding_07_min),
};

// Count the slides with this variable
static int slideCounter = 0;

void setup()
{
    // Init Serial for communication
    Serial.begin(115200);

    // Set pin mode for buttons
    pinMode(INKPLATE_USER1, INPUT_PULLUP);
    pinMode(INKPLATE_USER2, INPUT_PULLUP);

    // Init Inkplate class in 1-bit mode
    inkplate.begin(INKPLATE_1BW);

    // Init Inkplate test class (this just gives it pointer to the Inkplate object and params)
    testClass.init(&inkplate, testsDoneEepromOffset, wifiSSID, wifiPASS, qwiicTestAddress);

    // Check if tests have been completed with this offset
    if (!testClass.areTestsDone(testsDoneEepromOffset, testsDoneEepromValue))
    {
        // If tests are NOT done yet...
        // Perform tests in enclosure
        if (!testClass.testInEnclosure())
        {
            // Fatal error happened, don't continue the test - just inform the user via Serial
            // Test result should also display on the e-Paper so this is just a precaution
            Serial.println("Critical error: one of the tests failed!");
            Serial.println("Test stopping!");
            // Go to infinite loop
            while (true)
                ;
        }

        // e-Paper power supply test, VCOM set, and some other tests are done in Inkplate_6_Motion_VCOM_Set.ino

        // We're here - tests are done, mark it in memory
        testClass.writeEepromValue(testsDoneEepromOffset, testsDoneEepromValue);
    }
    // Show first slide
    displaySlide(slideCounter);
}

void loop()
{
    // Loop essentially controls the slideshow

    // USER2 button goes to previous slide
    if (!digitalRead(INKPLATE_USER2))
    {
        slideCounter--;
        if (slideCounter < 0)
            slideCounter = 8;

        // Show the slide
        displaySlide(slideCounter);

        // Slight debounce
        delay(5);
    }

    // USER1 button goes to next slide
    if (!digitalRead(INKPLATE_USER1))
    {
        slideCounter++;
        if (slideCounter > 8)
            slideCounter = 0;

        // Show the slide
        displaySlide(slideCounter);

        // Slight debounce
        delay(5);
    }
}


// Show a slide in the onboarding sequence
void displaySlide(int _slideIndex)
{
    // Check for range
    if (_slideIndex >= 10 || _slideIndex < 0)
        return;

    // Slides 2 and 4 are in bw mode
    if (_slideIndex == 2)
    {
        // Set mode to BW
        inkplate.selectDisplayMode(INKPLATE_BLACKWHITE);
        // Draw the image from memory
        // These are not JPG's but bitmaps
        inkplate.drawBitmap(0, 0, onboarding_01_min, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        inkplate.display(); // Do full refresh with bw slides
    }
    else if (_slideIndex == 4)
    {
        // Set mode to BW
        inkplate.selectDisplayMode(INKPLATE_BLACKWHITE);
        // Draw the image from memory
        inkplate.drawBitmap(0, 0, onboarding_03_min, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        inkplate.display();
        // This slide displays the animation
        // This function returns true if USER1  was pressed
        // It returns false if USER2  was pressed
        if (partialUpdateAnimation())
        {
            _slideIndex++;
            return;
        }
        else
        {
            _slideIndex--;
            return;
        }
    }
    else
    {
        // Other slides are in grayscale
        // Set mode to Grayscale
        inkplate.selectDisplayMode(INKPLATE_GRAYSCALE);
        // Draw the image from a jpg file
        inkplate.image.drawFromBuffer(onboardingSlides[_slideIndex], slideSizes[_slideIndex], 0, 0, 0, 0,
                                      INKPLATE_IMAGE_DECODE_FORMAT_JPG);
        if (_slideIndex == 0 || _slideIndex == 3 || _slideIndex == 5)
            // Do full refreshes after slides 2 and 4 and on the first slide
            inkplate.display();
        else
            // On other slides do partial update
            inkplate.partialUpdate();
    }
}

// Returns true if going to next slide, false if going to previous slide
bool partialUpdateAnimation()
{
    // To count the number of updates
    int numUpdates = 0;
    // Can be pretty liberal with the number of partial updates as the affected screen area is always moving
    int maxPartialUpdates = 100;

    // Some basic coordinations and dimensions of the animation
    int x = 18;
    int y = 212;
    int dx = 10; // Change in x direction
    int dy = 10; // Change in y direction
    int width = 979;
    int height = 519;

    // The logo which gets bounced around is also in slide_03.h
    int logoWidth = 92;
    int logoHeight = 121;

    while (true)
    {
        // Clear the previous position
        inkplate.fillRect(18 + 3, 212 + 3, width, height, WHITE);

        // Update the position of the logo
        x += dx;
        y += dy;

        // Check for button press to exit the function
        // A bit hacky but this is done multiple times during this while loop to increase responsiveness
        if (digitalRead(INKPLATE_USER1) == LOW)
            return true;
        if (digitalRead(INKPLATE_USER2) == LOW)
            return false;

        // Check for collisions with the edges and reverse direction if necessary
        if (x <= 18 || x + logoWidth >= 18 + width)
        {
            dx = -dx;
        }
        if (y <= 212 || y + logoHeight >= 212 + height)
        {
            dy = -dy;
        }

        if (digitalRead(INKPLATE_USER1) == LOW)
            return true;
        if (digitalRead(INKPLATE_USER2) == LOW)
            return false;

        // Draw the logo at the new position
        inkplate.drawBitmap(x, y, solderedLogoSmall, logoWidth, logoHeight, BLACK);
        inkplate.partialUpdate(true);
        numUpdates++;

        if (digitalRead(INKPLATE_USER1) == LOW)
            return true;
        if (digitalRead(INKPLATE_USER2) == LOW)
            return false;

        if (numUpdates >= maxPartialUpdates)
        {
            numUpdates = 0;
            inkplate.display();
        }

        if (digitalRead(INKPLATE_USER1) == LOW)
            return true;
        if (digitalRead(INKPLATE_USER2) == LOW)
            return false;
    }
}