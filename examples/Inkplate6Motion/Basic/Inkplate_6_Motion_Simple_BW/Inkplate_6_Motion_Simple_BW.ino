/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Simple_BW.ino
 * @brief       How to draw basic graphics and text in black and white (1 bit mode)
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

Inkplate inkplate; // Create Inkplate object

// Delay in milliseconds between different drawing examples
#define DELAY_MS 5000

void setup()
{
    inkplate.begin(INKPLATE_BLACKWHITE); // Initialize Inkplate in black-and-white mode
    inkplate.clearDisplay();             // Clear any data that may have been in (software) frame buffer.
    inkplate.display();                  // Update the display, important!
    // The display function is required to change the contents of the display!

    // Print welcome message and display it
    inkplate.setCursor(150, 400);
    inkplate.setTextSize(4);
    inkplate.print("Welcome to Inkplate 6MOTION!");
    inkplate.display();

    delay(DELAY_MS);
}

void loop()
{
    // Drawing a single pixel
    inkplate.clearDisplay();
    displayCurrentAction("Drawing a pixel");
    inkplate.drawPixel(100, 50, BLACK);
    inkplate.display();
    delay(DELAY_MS);

    // Drawing random pixels
    inkplate.clearDisplay();
    for (int i = 0; i < 600; i++)
    {
        inkplate.drawPixel(random(0, 1023), random(0, 757), BLACK);
    }
    displayCurrentAction("Drawing 600 random pixels");
    inkplate.display();
    delay(DELAY_MS);

    // Draw diagonal lines
    inkplate.clearDisplay();
    inkplate.drawLine(0, 0, 1023, 757, BLACK);
    inkplate.drawLine(1023, 0, 0, 757, BLACK);
    displayCurrentAction("Drawing two diagonal lines");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random lines
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.drawLine(random(0, 1023), random(0, 757), random(0, 1023), random(0, 757), BLACK);
    }
    displayCurrentAction("Drawing 50 random lines");
    inkplate.display();
    delay(DELAY_MS);

    // Drawing a grid
    inkplate.clearDisplay();
    for (int i = 0; i < 1024; i += 8)
    {
        inkplate.drawFastVLine(i, 0, 758, BLACK);
    }
    for (int i = 0; i < 758; i += 4)
    {
        inkplate.drawFastHLine(0, i, 1024, BLACK);
    }
    displayCurrentAction("Drawing a grid using horizontal and vertical lines");
    inkplate.display();
    delay(DELAY_MS);

    // Draw rectangle
    inkplate.clearDisplay();
    inkplate.drawRect(300, 300, 400, 300, BLACK);
    displayCurrentAction("Drawing rectangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.drawRect(random(0, 1023), random(0, 757), 100, 150, BLACK);
    }
    displayCurrentAction("Drawing many rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw filled rectangle
    inkplate.clearDisplay();
    inkplate.fillRect(300, 300, 400, 300, BLACK);
    displayCurrentAction("Drawing filled rectangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random filled rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.fillRect(random(0, 1023), random(0, 757), 30, 30, BLACK);
    }
    displayCurrentAction("Drawing many filled rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw circle
    inkplate.clearDisplay();
    inkplate.drawCircle(512, 379, 75, BLACK);
    displayCurrentAction("Drawing a circle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random circles
    inkplate.clearDisplay();
    for (int i = 0; i < 40; i++)
    {
        inkplate.drawCircle(random(0, 1023), random(0, 757), 25, BLACK);
    }
    displayCurrentAction("Drawing many circles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw filled circle
    inkplate.clearDisplay();
    inkplate.fillCircle(512, 379, 75, BLACK);
    displayCurrentAction("Drawing filled circle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random filled circles
    inkplate.clearDisplay();
    for (int i = 0; i < 40; i++)
    {
        inkplate.fillCircle(random(0, 1023), random(0, 757), 15, BLACK);
    }
    displayCurrentAction("Drawing many filled circles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw rounded rectangle
    inkplate.clearDisplay();
    inkplate.drawRoundRect(300, 300, 400, 300, 10, BLACK);
    displayCurrentAction("Drawing rounded rectangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random rounded rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.drawRoundRect(random(0, 1023), random(0, 757), 100, 150, 5, BLACK);
    }
    displayCurrentAction("Drawing many rounded rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw filled rounded rectangle
    inkplate.clearDisplay();
    inkplate.fillRoundRect(300, 300, 400, 300, 10, BLACK);
    displayCurrentAction("Drawing filled rounded rectangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random filled rounded rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.fillRoundRect(random(0, 1023), random(0, 757), 30, 30, 3, BLACK);
    }
    displayCurrentAction("Drawing many filled rounded rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw triangle
    inkplate.clearDisplay();
    inkplate.drawTriangle(300, 500, 700, 500, 512, 200, BLACK);
    displayCurrentAction("Drawing triangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw filled triangle
    inkplate.fillTriangle(350, 467, 650, 467, 512, 250, BLACK);
    displayCurrentAction("Drawing filled triangle");
    inkplate.display();
    delay(DELAY_MS);

    // Draw text with display rotation
    int r = 0;
    inkplate.setTextSize(5);
    inkplate.setTextColor(WHITE, BLACK);
    while (true)
    {
        inkplate.setCursor(100, 100);
        inkplate.clearDisplay();
        inkplate.setRotation(r);
        inkplate.print("INKPLATE 6MOTION");
        inkplate.display();
        r++;
        delay(DELAY_MS);
    }
}

// Function to indicate current action on display
void displayCurrentAction(String text)
{
    // Set text color to black, set the size and cursor position
    inkplate.setTextColor(BLACK);
    inkplate.setTextSize(2);
    inkplate.setCursor(2, 740);
    // Draw white background before printing
    inkplate.fillRect(0, 740, 1024, 18, WHITE);
    inkplate.print(text); // Print the text
}
