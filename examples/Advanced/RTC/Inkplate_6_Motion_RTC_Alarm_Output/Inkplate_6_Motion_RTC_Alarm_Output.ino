/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_RTC_Alarm_Output.ino
 * @brief       This example will send a pulse on the PC13 pin upon a RTC alarm event
 *              Connect a 330Ohm resistor and an LED on PC13, upon the alarm, the LED will blink
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

// Set alarm to active 12:24:35 same day
uint8_t alarmDay = 29;
uint8_t alarmHour = 12;
uint8_t alarmMinute = 24;
uint8_t alarmSeconds = 35;

// Set alarm to be once every day
// uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY;

// Set alarm to be once every hour
// uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS;

// Set alarm to be once every minute
uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES;

// Checks for change in seconds in order to update print on screen
uint8_t oldSeconds = seconds;

// Variable keeps track how many times screen was update with partial updates
int partialUpdateCount = 0;

void setup()
{
    Serial.begin(115200);

    // Initialize Inkplate Motion Arduino Library
    inkplate.begin();

    // Clear the screen
    inkplate.display();

    // Load Soldered Font
    inkplate.setFont(&solderedFont30pt7b);

    // Initialize RTC library and set it to 24 hour format and reset RTC
    inkplate.rtc.begin(RTC_HOURFORMAT_24, true);

    // To use 12 hour format use this initializer
    // inkplate.rtc.begin(RTC_HOURFORMAT_12, true);

    // Set time & date on RTC
    inkplate.rtc.setTime(hours, minutes, seconds, subSeconds);
    inkplate.rtc.setDate(day, month, year, weekday);

    // Enable alarm on RTC Alarm A (Alarm B is still not fully supported yet!)
    inkplate.rtc.enableSimpleAlarm(alarmDay, alarmHour, alarmMinute, alarmSeconds, RTC_ALARM_A, alarmMask);

    // Enable pulse on PC13 on alarm event on alarm A (alarm B is not fully supported yet!)
    inkplate.rtc.setAlarmOutput(true, RTC_OUTPUT_ALARMA);
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

    // If second passed, print out new time and date
    if (s != oldSeconds)
    {
        // Clear display and set the text cursor to X = 80, Y = 390
        inkplate.clearDisplay();
        inkplate.setCursor(80, 390);

        // Update the variable for the seconds to avoid constant screen refresh
        oldSeconds = s;

        // Print out the time and date on the Inkplate
        inkplate.print("Time:");
        // Print out hours
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

        // Use polling method to detect alarm event and do not clear alarm flag rightaway
        // (longer pulse time to see LED blink)
        if (inkplate.rtc.checkForAlarm(false))
        {
            // Wait a little bit so we can see the LED bink
            delay(250);

            // Now clear alarm flag (you don't need to pass true argument in function, it's optional)
            inkplate.rtc.checkForAlarm(true);
            
            // Print out the message
            inkplate.setCursor(320, 450);
            inkplate.println("ALARM EVENT!");
        }

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

// Helper function to printout number with leading zeros
void printWithLeadingZeros(uint32_t _number, int _leadingZeros)
{
    for (int i = (_leadingZeros - 1); i >= 0; i--)
    {
        uint32_t _divider = pow(10, i);
        inkplate.print((_number / _divider) % 10, DEC);
    }
}