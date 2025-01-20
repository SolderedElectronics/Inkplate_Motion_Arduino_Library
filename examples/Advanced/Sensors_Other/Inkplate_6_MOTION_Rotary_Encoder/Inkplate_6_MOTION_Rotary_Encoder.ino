/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_Rotary_Encoder.ino
 * @brief       If you have an Inkplate housing, here's how to use the onboard magnetic rotary encoder
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Create Inkplate Motion Arduino Library object.
Inkplate inkplate;

// Variables hold current and previous angle of the rotary encoder.
int currentValue = 0;
int oldValue = 0;

// Fast and easy conversion from degrees to radians.
const float DEG2RAD = PI / 180.0f;

void setup() {
  // Initialize Inkplate Motion library.
  inkplate.begin(INKPLATE_BLACKWHITE);

  // Do a full update each 60 partial updates
  inkplate.setFullUpdateTreshold(60);

  // Enable power to the rotary encoder.
  inkplate.peripheralState(INKPLATE_ROTARY_ENCODER_PERIPH, true);
  // Initialize rotary encoder.
  inkplate.rotaryEncoder.begin();

  // Do initial print of position
  printPosition(currentValue, oldValue);

  // Refresh the display (clear everything from the screen).
  inkplate.display();
}

void loop() {
  // Get the current position of the rotary encoder.
  currentValue = (int)(inkplate.rotaryEncoder.rawAngle() * AS5600_RAW_TO_DEGREES);

  // Threshold is 2 degrees to update the position of the line on the screen.
  if (abs(currentValue - oldValue) >= 2) {
    // Clear what's previously in the b uffer
    inkplate.clearDisplay();

    // Update the position of the line.
    printPosition(oldValue, currentValue);

    // Update the screen (keep the e-paper power supply active all the time - faster screen refresh!).
    inkplate.partialUpdate(true);

    // Update the old value.
    oldValue = currentValue;
  }
}

void printPosition(int _oldPosition, int _newPos) {
  // Redraw the circle.
  inkplate.drawCircle(inkplate.width() / 2, inkplate.height() / 2, 310, BLACK);

  // Draw the new position.
  int _x2 = (300 * cos(DEG2RAD * _newPos)) + (inkplate.width() / 2);
  int _y2 = (300 * sin(DEG2RAD * _newPos)) + (inkplate.height() / 2);
  // Thicken the line
  inkplate.drawLine(inkplate.width() / 2, inkplate.height() / 2, _x2, _y2, BLACK);
}
