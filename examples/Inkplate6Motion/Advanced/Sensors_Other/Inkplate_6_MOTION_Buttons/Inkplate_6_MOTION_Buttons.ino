/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_Buttons.ino
 * @brief       Simply use the onboard buttons on Inkplate 6MOTION
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Library.
#include <InkplateMotion.h>


Inkplate inkplate;  // Create object on Inkplate library

void setup() {
  inkplate.begin();         // Init library
  inkplate.clearDisplay();  // Clear any data that may have been in (software) frame buffer
  // Print info text
  inkplate.setTextSize(3);  // Make the text a bit bigger so it's visible
  inkplate.setCursor(200, 200);
  inkplate.print("Buttons example - press the user buttons!");
  inkplate.display();  // Clear the display


  // Set button pinmodes
  pinMode(INKPLATE_USER1, INPUT_PULLUP);
  pinMode(INKPLATE_USER2, INPUT_PULLUP);
  pinMode(INKPLATE_WAKE, INPUT_PULLUP);
}

void loop() {
  inkplate.clearDisplay();
  inkplate.setCursor(200, 200);
  inkplate.print("Buttons example - press the user buttons!");
  inkplate.setCursor(200, 450);
  int pressedButton = getButtonPress();
  if (pressedButton == 1) {
    inkplate.println("USER 1 pressed!");
  } else if (pressedButton == 2) {
    inkplate.println("USER 2 pressed!");
  } else if (pressedButton == 3) {
    inkplate.println("WAKE pressed!");
  }
  inkplate.partialUpdate();  // Update the display
}

int getButtonPress() {
  // Wait for button press and return which button was pressed accordingly
  while (true) {
    if (digitalRead(INKPLATE_USER1) == LOW)
      return 1;
    if (digitalRead(INKPLATE_USER2) == LOW)
      return 2;
    if (digitalRead(INKPLATE_WAKE) == LOW)
      return 3;

    delay(5);  // Short debounce delay
  }
}