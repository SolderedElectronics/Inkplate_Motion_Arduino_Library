// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>
// Include slides for the onboarding sequence
#include "slides/onboarding_00_min.h"
#include "slides/onboarding_01_min.h"
#include "slides/onboarding_02_min.h"
#include "slides/onboarding_03_min.h"
#include "slides/onboarding_04_min.h"
#include "slides/onboarding_05_min.h"
#include "slides/onboarding_06_min.h"
#include "slides/onboarding_07_min.h"
#include "slides/onboarding_initial_min.h"

// Create an Inkplate object.
Inkplate inkplate;

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

// Count the slides
static int slideCounter = 0;

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
        inkplate.drawBitmap(0, 0, onboarding_01_min, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        inkplate.display(); // Do full refresh with bw slides
    }
    else if (_slideIndex == 4)
    {
        // Set mode to BW
        inkplate.selectDisplayMode(INKPLATE_BLACKWHITE);
        // Draw the image from memory
        inkplate.drawBitmap(0, 0, onboarding_03_min, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        inkplate.display(); // Do full refresh with bw slides
    }
    else
    {
        // Other slides are in grayscale
        // Set mode to Grayscale
        inkplate.selectDisplayMode(INKPLATE_GRAYSCALE);
        // Draw the image
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

void setup()
{
    // Initialize Inkplate Motion Library in grayscale mode.
    inkplate.begin(INKPLATE_GL16);

    // Set pin mode for butotns
    pinMode(INKPLATE_USER1, INPUT_PULLUP);
    pinMode(INKPLATE_USER2, INPUT_PULLUP);

    // Show first slide
    displaySlide(slideCounter);
}

void loop()
{
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
