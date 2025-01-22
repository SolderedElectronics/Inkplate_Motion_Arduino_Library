/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_SD_Read_Write_Text.ino
 * @brief       Write and read from a .txt file on the SD card
 *
 *              Use the official SD card formatter: https://www.sdcard.org/downloads/formatter/
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Incldue Inkplate Motion library
#include "InkplateMotion.h"

// Create Inkplate Motion object
Inkplate inkplate;

// Define the string to write
const char *testString = "SD Card test string to write and read from Inkplate 6MOTION. How you doin'?!";

void setup()
{
    // Init Inkplate in black and white mode
    inkplate.begin(INKPLATE_BLACKWHITE);

    // Set text printing options
    inkplate.setTextColor(BLACK);
    inkplate.setTextSize(2);

    // Initialize microSD card
    if (!inkplate.microSDCardInit())
    {
        // Print error message if SD card could not be initialized
        inkplate.print("Couldn't init SD card!");
        inkplate.partialUpdate();
        // Go to infinite loop
        while (1)
            ;
    }


    // Let's create the .txt file and write to it
    // This will create the file if it doesn't exist, and open it in read-write mode if it exists:
    File file = inkplate.sdFat.open("testFile.txt", O_CREAT | O_RDWR);
    if (!file)
    {
        // Print error message if the file couldn't be created
        inkplate.print("Couldn't create the file on SD card!");
        inkplate.partialUpdate();
        // Go to infinite loop
        while (1)
            ;
    }
    // Write to the file and close it
    file.print(testString);
    file.close();

    delay(250); // Wait a bit...

    // Read back from the file
    // This will only open the file in read only mode:
    file = inkplate.sdFat.open("testFile.txt", O_RDONLY);
    if (!file)
    {
        // Print error message if the file couldn't be opened back
        inkplate.print("Couldn't open the created file!");
        inkplate.partialUpdate();
        inkplate.sdFat.remove("testFile.txt"); // Ensure file is deleted
        // Go to infinite loop
        while (1)
            ;
    }

    // Read file contents to buffer
    char buffer[128];
    int bytesRead = file.read(buffer, sizeof(buffer) - 1);
    file.close();
    // Bytes have been read
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0'; // Null-terminate the c-style string
        // Let's print it to Inkplate!
        inkplate.println("\nFile contents:\n");
        inkplate.println(buffer);
    }
    else
    {
        // Print error message if the file couldn't be opened back
        inkplate.print("Couldn't read the test string!");
    }
    // Update the display with the result of the sketch
    inkplate.partialUpdate();

    // Delete the file
    inkplate.sdFat.remove("testFile.txt");
}

void loop()
{
}