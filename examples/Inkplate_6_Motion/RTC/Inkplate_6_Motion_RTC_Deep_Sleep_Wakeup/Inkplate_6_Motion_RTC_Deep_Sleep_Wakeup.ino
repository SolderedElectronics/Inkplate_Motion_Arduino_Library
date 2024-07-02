/*
   This example shows how to use STM32H743 (Inkplate NextGen) internal RTC to wake STM32 up from sleep.

*/
// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Include custom font (Soldered Font).
#include "solderedFont30pt7b.h"

// Create Inkplate Motion Library object.
Inkplate inkplate;

// Setting the clock: 12:24:25.
uint8_t hours = 12;
uint8_t minutes = 24;
uint8_t seconds = 25;
uint32_t subSeconds = 0;

// Setting the date: 29/5/2024, Monday.
uint8_t day = 29;
uint8_t month = 5;
uint8_t year = 24;

// Recommended way of def. day of week.
uint8_t weekday = RTC_WEEKDAY_MONDAY;

// Not recommended way of def. day of week, but it works.
// uint8_t weekday = 1;

// Set alarm to active 12:24:35 same day.
uint8_t alarmDay = 29;
uint8_t alarmHour = 12;
uint8_t alarmMinute = 24;
uint8_t alarmSeconds = 35;

// Set alarm to be once every day.
// uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY;

// Set alarm to be once every hour.
// uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS;

// Set alarm to be once every minute.
uint32_t alarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES;

void setup()
{
    // Initialize Inkplate Motion Arduino Library.
    inkplate.begin();

    // Clear the screen.
    inkplate.display();

    // Load Soldered Font.
    inkplate.setFont(&solderedFont30pt7b);

    // Initialize RTC library and set it to 24 hour format.
    inkplate.rtc.begin(RTC_HOURFORMAT_24);

    // Clear alarm flag.
    inkplate.rtc.checkForAlarm(true);

    // To use 12 hour format use this initializer.
    // inkplate.rtc.begin(RTC_HOURFORMAT_12, true);

    // Check if the time is already set. If is not, set it!
    if (!inkplate.rtc.isTimeSet())
    {
        inkplate.rtc.setTime(hours, minutes, seconds, subSeconds);
        inkplate.rtc.setDate(day, month, year, weekday);

        // Enable alarm on RTC Alarm A (Alarm B is still not fully supported yet!).
        inkplate.rtc.enableAlarm(alarmDay, alarmHour, alarmMinute, alarmSeconds, RTC_ALARM_A, alarmMask);

        // Enable interrupt on alarm event (also waking up from sleep).
        // We don't want to call any function on wakeup, so we send NULL as argument.
        inkplate.rtc.enableAlarmInterrupt(NULL);
    }

    // Print new time.
    printTime();

    // Refresh the screen.
    inkplate.display();

    // Disable USB voltage detection on USB OTG (This causes high current consumption in sleep!).
    HAL_PWREx_DisableUSBVoltageDetector();

    // Use different voltage scale for internal voltage regulator (lower current consumption in sleep mode).
    HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);

    // Finally, enter in sleep mode (deep sleep, lowest current consumption, but data is not retained, after wake up,
    // code starts from begining).
    deepSleep();
}

void loop()
{
    // Loop must be empty! No code should be here. Everything needs to be inside setup!
}

void deepSleep()
{
    HAL_PWREx_EnterSTANDBYMode(PWR_D3_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D1_DOMAIN);
}

void printTime()
{
    // Variables for time and date.
    uint8_t h, m, s, d, mn, y, wk;
    uint32_t ss;

    // Get time and date data from STM32 internal RTC using pointers.
    // First NULL is for PM/AM indicator (not used in 24 hour mode) and second is for Daylight
    // Saving (not used in this example).
    inkplate.rtc.getTime(&h, &m, &s, &ss, NULL, NULL);
    inkplate.rtc.getDate(&d, &mn, &y, &wk);

    // Clear display and set the text cursor to X = 80, Y = 390.
    inkplate.clearDisplay();
    inkplate.setCursor(80, 390);

    // Print out the time and date on the Inkplate.
    inkplate.print("Time:");
    // Print out hours.
    inkplate.print(h, DEC);
    inkplate.print(':');

    // Print out first and second digit of minutes.
    printWithLeadingZeros(m, 2);
    inkplate.print(':');

    // Print out first and second digit of seconds.
    printWithLeadingZeros(s, 2);
    inkplate.print(';');
    // Print subseconds (if ss = 255 it means 0ms,
    // if ss = 0 it menas 999ms).
    printWithLeadingZeros((int)((1 - (ss / 256.0)) * 100), 2);

    // Create the space between time and date.
    inkplate.print("  ");

    // Print out date (by European date format).
    inkplate.print("Date:");
    printWithLeadingZeros(d, 2);
    inkplate.print('.');
    printWithLeadingZeros(mn, 2);
    inkplate.print('.');
    printWithLeadingZeros(y + 2000, 4);
    inkplate.println('.');
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