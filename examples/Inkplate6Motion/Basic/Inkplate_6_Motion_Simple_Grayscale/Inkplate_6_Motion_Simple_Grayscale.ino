/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Simple_Grayscale.ino
 * @brief       Example for drawing basic graphics in grayscale
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion library
#include <InkplateMotion.h>

Inkplate inkplate; // Create Inkplate object

// Delay in milliseconds between different drawing examples
#define DELAY_MS 5000

void setup()
{
    inkplate.begin(INKPLATE_GRAYSCALE); // Initialize Inkplate in grayscale mode (3-bit)
    inkplate.clearDisplay();       // Clear frame buffer
    inkplate.display();            // Clear the screen

    // Draw text  and display it
    // Colors range from 0 (black) to 15 (white)
    inkplate.setTextColor(0, 15); // Black text on white background
    inkplate.setCursor(150, 400);
    inkplate.setTextSize(4);
    inkplate.print("Welcome to Inkplate 6MOTION!");
    inkplate.display();
    delay(5000);
}

void loop()
{
    // Drawing a single pixel
    inkplate.clearDisplay();
    displayCurrentAction("Drawing a pixel");
    inkplate.drawPixel(512, 379, 0); // Black pixel at center
    inkplate.display();
    delay(DELAY_MS);

    // Drawing random grayscale pixels
    inkplate.clearDisplay();
    for (int i = 0; i < 1000; i++)
    {
        inkplate.drawPixel(random(0, 1023), random(0, 757), random(0, 15));
    }
    displayCurrentAction("Drawing random grayscale pixels");
    inkplate.display();
    delay(DELAY_MS);

    // Draw diagonal lines
    inkplate.clearDisplay();
    inkplate.drawLine(0, 0, 1023, 757, 0);  // Black diagonal line
    inkplate.drawLine(1023, 0, 0, 757, 15); // White diagonal line
    displayCurrentAction("Drawing diagonal lines");
    inkplate.display();
    delay(DELAY_MS);

    // Draw random lines
    inkplate.clearDisplay();
    for (int i = 0; i < 50; i++)
    {
        inkplate.drawLine(random(0, 1023), random(0, 757), random(0, 1023), random(0, 757), random(0, 15));
    }
    displayCurrentAction("Drawing random lines");
    inkplate.display();
    delay(DELAY_MS);

    // Draw rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 10; i++)
    {
        int x = random(0, 924);
        int y = random(0, 657);
        int w = random(50, 100);
        int h = random(50, 100);
        inkplate.drawRect(x, y, w, h, random(0, 15));
    }
    displayCurrentAction("Drawing random rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw filled rectangles
    inkplate.clearDisplay();
    for (int i = 0; i < 10; i++)
    {
        int x = random(0, 924);
        int y = random(0, 657);
        int w = random(50, 100);
        int h = random(50, 100);
        inkplate.fillRect(x, y, w, h, random(0, 15));
    }
    displayCurrentAction("Drawing filled rectangles");
    inkplate.display();
    delay(DELAY_MS);

    // Draw text in different sizes and shades
    inkplate.clearDisplay();
    for (int i = 0; i < 6; i++)
    {
        inkplate.setTextColor(i);
        inkplate.setTextSize(i + 1);
        inkplate.setCursor(100, 100 + i * 100);
        inkplate.print("Inkplate 6MOTION");
    }
    displayCurrentAction("Drawing text in different sizes and shades");
    inkplate.display();
    delay(DELAY_MS);

    // Draw text with display rotation
    int r = 0;
    inkplate.setTextSize(5);
    inkplate.setTextColor(15, 0);
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
    inkplate.setTextColor(0);
    inkplate.setTextSize(2);
    inkplate.setCursor(2, 740);
    // Draw white background before printing
    inkplate.fillRect(0, 740, 1024, 18, 15);
    inkplate.print(text); // Print the text
}
