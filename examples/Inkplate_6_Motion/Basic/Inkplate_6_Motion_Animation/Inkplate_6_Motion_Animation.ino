// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Include header file for all animation frames.
#include "animationFrames.h"

// Create Inkplate Motion Library object.
Inkplate inkplate;

// Variable holds current frame in the animation.
int currentFrame = 0;

void setup()
{
    // Initialize Serial communication.
    Serial.begin(115200);

    // Initialize Inkplate library. Set Inkplate library into 1 bit mode.
    inkplate.begin(INKPLATE_1BW);

    // Do a full update to clear the dispaly.
    inkplate.display(true);

    delay(1500);
}

void loop()
{
    // Keep updating the animation (push new frames).
    showAnimaiton(&inkplate, animationFrames, &currentFrame, animationFramesTotal);
}

void showAnimaiton(Inkplate *_inkplate, const uint8_t **_frames, int *_currentFrame, int _maxFrames)
{
    // NOTE NOTE NOTE NOTE!
    // This function only wokrs with full size images (1024 x 758).
    _inkplate->drawBitmapFast((const uint8_t *)_frames[(*_currentFrame)]);

    // Updatew the variable for frame index.
    (*_currentFrame)++;

    // Roll back if needed.
    if ((*_currentFrame) >= _maxFrames)
    {
        (*_currentFrame) = 0;
    }

    // Do a partial update and keep ePaper power supply active (to speed up the display update process).
    _inkplate->partialUpdate(true);
}