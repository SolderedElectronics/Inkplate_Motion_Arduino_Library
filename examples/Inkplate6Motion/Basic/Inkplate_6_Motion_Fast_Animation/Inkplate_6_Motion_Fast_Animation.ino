/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Fast_Animation.ino
 * @brief       How to use partial update in 1 bit mode for the fastest display update
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Library
#include <InkplateMotion.h>

// For now, animation frames for usage with drawBitmapFast can only be images exactly 1024 x 758

// Include header file for all animation frames
// To select the animation, open this file and edit it to select the animation
#include "animationFrames.h"

// Create Inkplate Motion Library object
Inkplate inkplate;

// Variable holds index of the current frame in the animation.
int currentFrame = 0;

void setup()
{
    // Initialize Inkplate library, set Inkplate library into 1 bit, black and white mode
    inkplate.begin(INKPLATE_BLACKWHITE);

    // Do a full update to clear the display
    // The parameter set to 'true' is leaveOn, it will leave the e-Paper on
    // This makes the update faster
    inkplate.display(true);

    // Wait a bit
    delay(1500);
}

void loop()
{
    // Keep updating the animation (push new frames)
    showAnimaiton(&inkplate, animationFrames, &currentFrame, animationFramesTotal);
}

void showAnimaiton(Inkplate *_inkplate, const uint8_t **_frames, int *_currentFrame, int _maxFrames)
{
    // Important note: this function only works with full size images (1024 x 758)
    _inkplate->drawBitmapFast((const uint8_t *)_frames[(*_currentFrame)]);

    // Updatew the variable for frame index
    (*_currentFrame)++;

    // Roll back if needed
    if ((*_currentFrame) >= _maxFrames)
        (*_currentFrame) = 0;

    // Do a partial update and keep ePaper power supply active
    _inkplate->partialUpdate(true);
}