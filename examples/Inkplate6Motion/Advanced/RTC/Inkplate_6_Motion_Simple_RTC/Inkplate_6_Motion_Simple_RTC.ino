/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Fast_Animation.ino
 * @brief       How get internal RTC time & date date on Inkplate 6MOTION
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion Arduino Library
#include <InkplateMotion.h>

// Include custom font (Soldered Font)
#include "solderedFont30pt7b.h"

// Create Inkplate Motion Library object
Inkplate inkplate;

// Setting the clock: 12:24:25
uint8_t hours = 12;
uint8_t minutes = 24;
uint8_t seconds = 25;
uint32_t subSeconds = 0;

// Setting the date: 29/5/2024, Monday
uint8_t day = 29;
uint8_t month = 5;
uint8_t year = 24;

// Recommended way of def. day of week
uint8_t weekday = RTC_WEEKDAY_MONDAY;

// Not recommended way of def. day of week, but it works
// uint8_t weekday = 1;

// Checks for change in seconds in order to update print on screen
uint8_t oldSeconds = seconds;

// Variable keeps track how many times screen was update with partial updates
int partialUpdateCount = 0;

void setup()
{
    // Initialize Inkplate Motion Arduino Library
    inkplate.begin();

    // Clear the screen
    inkplate.display();

    // Load Soldered Font
    inkplate.setFont(&solderedFont30pt7b);

    // Initialize RTC library and set it to 24 hour format
    inkplate.rtc.begin(RTC_HOURFORMAT_24);

    // To use 12 hour format use this initializer
    // inkplate.rtc.begin(RTC_HOURFORMAT_12);

    // To reset RTC with every reset of the MCU, set second argument in begin function to true
    // NOTE: This also works for 12 hour format! :)
    // inkplate.rtc.begin(RTC_HOURFORMAT_24, true);

    // Check if the time is already set. If is not, set it!
    if (!inkplate.rtc.isRTCSet())
    {
        inkplate.rtc.setTime(hours, minutes, seconds, subSeconds);
        inkplate.rtc.setDate(day, month, year, weekday);
    }
}

void loop()
{
    // Variables for time and date
    uint8_t h, m, s, d, mn, y, wk;
    uint32_t ss;

    // Get time and date data from STM32 internal RTC using pointers
    // First NULL is for PM/AM indicator (not used in 24 hour mode) and second is for Daylight
    // Saving (not used in this example)
    inkplate.rtc.getTime(&h, &m, &s, &ss, NULL, NULL);
    inkplate.rtc.getDate(&d, &mn, &y, &wk);

    // If seconds has passed, print out new time and date
    if (s != oldSeconds)
    {
        // Clear display and set the text cursor to X = 80, Y = 390
        inkplate.clearDisplay();
        inkplate.setCursor(80, 390);

        // Update the variable for the seconds to avoid constant screen refresh
        oldSeconds = s;

        // Print out the time and date on the Inkplate
        inkplate.print("Time:");
        // Print out hours.
        inkplate.print(h, DEC);
        inkplate.print(':');

        // Print out first and second digit of minutes
        printWithLeadingZeros(m, 2);
        inkplate.print(':');

        // Print out first and second digit of seconds
        printWithLeadingZeros(s, 2);
        inkplate.print(';');
        // Print subseconds (if ss = 255 it means 0ms,
        // if ss = 0 it menas 999ms)
        printWithLeadingZeros((int)((1 - (ss / 256.0)) * 100), 2);

        // Create the space between time and date
        inkplate.print("  ");

        // Print out date (by European date format)
        inkplate.print("Date:");
        printWithLeadingZeros(d, 2);
        inkplate.print('.');
        printWithLeadingZeros(mn, 2);
        inkplate.print('.');
        printWithLeadingZeros(y + 2000, 4);
        inkplate.println('.');

        // Check if the screen must be updated with fill refresh or partial refresh
        if (partialUpdateCount > 20)
        {
            // Do a full update
            inkplate.display();

            // Reset the counter
            partialUpdateCount = 0;
        }
        else
        {
            // Update the screen and keep the power supply active
            inkplate.partialUpdate(true);

            // Update the counter for the parital updates
            partialUpdateCount++;
        }
    }
}

// Helper function to printout number with leading zeros.
void printWithLeadingZeros(uint32_t _number, int _leadingZeros)
{
    for (int i = (_leadingZeros - 1); i >= 0; i--)
    {
        uint32_t _divider = pow(10, i);
        inkplate.print((_number / _divider) % 10, DEC);
    }
}