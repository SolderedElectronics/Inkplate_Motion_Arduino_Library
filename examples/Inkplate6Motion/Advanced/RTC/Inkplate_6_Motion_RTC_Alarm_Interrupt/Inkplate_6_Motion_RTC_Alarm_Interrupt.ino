/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_RTC_Alarm_Interrupt.ino
 * @brief       How to use Inkplate 6MOTION's internal RTC to generate an interrupt on alarm
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

// Flag for alarm event (it has to be volatile)
volatile bool alarmFlag = false;

void setup()
{
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
    inkplate.rtc.enableAlarm(alarmDay, alarmHour, alarmMinute, alarmSeconds, RTC_ALARM_A, alarmMask);

    // Enable interrupt on alarm event
    // Watchout! This interrupt can wake up your MCU from sleep!
    inkplate.rtc.enableAlarmInterrupt(alarmFunction);
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

        // Check for alarm event
        if (alarmFlag)
        {
            // Clear the flag
            alarmFlag = false;

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

// Function is called everytime when alarm event happens
void alarmFunction()
{
    alarmFlag = true;
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