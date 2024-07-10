// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create Inkplate object.
Inkplate inkplate;

// Variable to store backup RAM data.
int value = 0;

void setup()
{
    // Initialize Serial communication @ 115200 bauds.
    Serial.begin(115200);

    // Write debug message.
    Serial.println("Inkplate started");

    // Initialize Inkplate Motion library.
    inkplate.begin();

    // Initialize STM32 RTC (needed for Backup RAM) without reseting the whole RTC.
    inkplate.rtc.begin(RTC_HOURFORMAT_24);

    // Check if is this first time the example is run, reset the variable.
    if (!inkplate.rtc.isRTCSet())
    {
        // Initialize RTC and reset it! Otherwise, backup RAM will return wrong value.
        inkplate.rtc.begin(RTC_HOURFORMAT_24, true);

        // Force RTC Set variable to run this part of the code only once.
        // The otherway to do this is to set the clock or date.
        inkplate.rtc.rtcSetFlag();

        // Set variable stored in backup RAM to zero. This RAM is available between
        // system resets, but also between power cycles (if RTC battery is provided).
        inkplate.rtc.writeToBackupRAM(0, &value, sizeof(value));
    }

    // Set text scaling to 4.
    inkplate.setTextSize(4); 

    // Set text color to black with white background. 
    inkplate.setTextColor(BLACK, WHITE);

    // Start printing text at X = 0, Y = 150.
    inkplate.setCursor(0, 150);

    // Print the reset cause.
    printResetCause();

    // Get the new value of the variable from the backup RAM.
    inkplate.rtc.readFromBackupRAM(0, &value, sizeof(value));

    // Print some text on screen.
    inkplate.printf("Inkplate 6 MOTION deep sleep example.\nWoken up %d times.", value++);

    // Store the new value.
    inkplate.rtc.writeToBackupRAM(0, &value, sizeof(value));

    // Refresh the screen.
    inkplate.display();

    // Check if the wake up from sleep did reset the board. If so, clear the flags.
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        // Clear Standby flag 
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

        // Wake-up button on Inkplate 6 Motion is PC13 = WAKEUP PIN 4. 
        HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
    }

    // Enable wake up button. By setting PC13 to low (button press), Inkplate will wake up.
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_LOW);

    // Print last message before goint to sleep.
    Serial.println("Going to sleep");
    Serial.flush();

    // Finally, enter in sleep mode (deep sleep, lowest current consumption, but data is not retained, after wake up, code starts from begining)
    deepSleep();
}

void loop()
{
    // Nothing! Must be empty!
}

void printResetCause(void)
{
    uint32_t reset_cause = RCC->RSR; // Read the RSR register

    inkplate.print("Reset cause(s):\n");
    
    if (reset_cause & RCC_RSR_CPURSTF)
    {
        inkplate.printf("CPU Reset\n");
    }
    
    if (reset_cause & RCC_RSR_D1RSTF)
    {
        inkplate.printf("Deep sleep reset (D1)\n");
    }
    
    if (reset_cause & RCC_RSR_D2RSTF)
    {
        inkplate.printf("Deep sleep reset (D2)\n");
    }

    if (reset_cause & RCC_RSR_BORRSTF)
    {
        inkplate.printf("Brownout reset\n");
    }
    
    if (reset_cause & RCC_RSR_PINRSTF)
    {
        inkplate.printf("Pin reset\n");
    }

    if (reset_cause & RCC_RSR_PORRSTF)
    {
        inkplate.printf("Power-on reset\n");
    }

    if (reset_cause & RCC_RSR_SFTRSTF)
    {
        inkplate.printf("Software reset\n");
    }

    if (reset_cause & RCC_RSR_IWDG1RSTF)
    {
        inkplate.printf("Independent watchdog reset\n");
    }

    if (reset_cause & RCC_RSR_WWDG1RSTF)
    {
        inkplate.printf("Window watchdog reset\n");
    }


    if (reset_cause & RCC_RSR_LPWRRSTF)
    {
        inkplate.printf("Low-power reset\n");
    }

    // Clear the reset flags
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

// Deep sleep function. Calling this will put STM32 in deep sleep mode. If all peripherals are disabled
// deep sleep current will be around 20-30uA.
void deepSleep()
{
    // Disable all peripherals including WiFi, microSD card, SDRAM, sensors etc.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_ALL, false);

    // Disable USB voltage detection on USB OTG (This causes high current consumption in sleep!)
    HAL_PWREx_DisableUSBVoltageDetector();

    // Use different voltage scale for internal voltage regulator (lower current consumption in sleep mode)
    HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);

    // Put every domain into standby mode.
    HAL_PWREx_EnterSTANDBYMode(PWR_D3_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D1_DOMAIN);
}
