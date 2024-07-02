// Include Inkplate Motion library.
#include <InkplateMotion.h>

void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);
    
    // Print debzug message.
    Serial.println("STM32 Code started.");
    Serial.flush();

    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        // Clear Standby flag 
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
    }

    delay(1000);

    Serial.println("Setting up a wake-pin");
    Serial.flush();

    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_LOW);

    Serial.println("Prepare to sleep");
    Serial.flush();

    // Disable USB voltage detection on USB OTG (This causes high current consumption in sleep!)
    HAL_PWREx_DisableUSBVoltageDetector();

    // Use different voltage scale for internal voltage regulator (lower current consumption in sleep mode)
    HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);

    Serial.println("Going to sleep");
    Serial.flush();

    // Finally, enter in sleep mode (deep sleep, lowest current consumption, but data is not retained, after wake up, code starts from begining)
    deepSleep();

}

void loop()
{

}

void deepSleep()
{
    HAL_PWREx_EnterSTANDBYMode(PWR_D3_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
    HAL_PWREx_EnterSTANDBYMode(PWR_D1_DOMAIN);
}