// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create Inkplate object.
Inkplate inkplate;

void setup()
{
    // Initialize Serial communication @ 115200 bauds.
    Serial.begin(115200);

    // Write debug message.
    Serial.println("Inkplate started");

    // Initialize Inkplate Motion library.
    inkplate.begin();

    // Set text scaling to 4.
    inkplate.setTextSize(4); 

    // Set text color to black with white background. 
    inkplate.setTextColor(BLACK, WHITE);

    // Start printing text at X = 50, Y = 350.
    inkplate.setCursor(50, 350);

    // Print some text on screen.
    inkplate.print("Inkplate 6 MOTION deep sleep example");

    // Refresh the screen.
    inkplate.display();

    // Check if the wake up from sleep did reset the board. If so, clear the flags.
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        // Clear Standby flag 
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
    }


    // Enable wake up button.
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

// Deep sleep function. Calling this 
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
