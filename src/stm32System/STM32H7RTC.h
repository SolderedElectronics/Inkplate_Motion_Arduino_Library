/**
 **************************************************
 *
 * @file        STM32H7RTC.h
 * @brief       Main header file for the STM32 built-in RTC
 *              peripheral. RTC uses exteranl clock source (external XTAL
 *              of 32.768kHz). Library Sets and gets the time, date, alarms,
 *              sets user Alarm callbacks, manages backup memory region etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Add a header guard for the STm32 RTC library.
#ifndef __STM32H7RTC_H__
#define __STM32H7RTC_H__

// Include main Arduino header file.
#include "Arduino.h"

// Default format for the RTC.
#define RTC_FORMAT RTC_FORMAT_BIN

// STM32 HAL Alarm Callback function.
extern "C" void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

// STM32 HAL Alarm Callback register.
extern "C" void RTC_Alarm_IRQHandler(void);

// STM32 Peripheral Initialization.
extern "C" void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc);

// STM32 Peripheral Deinitialization.
extern "C" void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc);

// STM32 RTC Instance.
static RTC_HandleTypeDef _stm32MotionHrtc = {0};

// STM32 RTC libary for the Inkplate Motion.
class STM32H7RTC
{
  public:
    // Library constructor.
    STM32H7RTC();

    // Library initializer.
    RTC_HandleTypeDef begin(uint8_t _format, bool _resetRtc = false);

    // Method set the time.
    void setTime(uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _ss, uint8_t _pmAm = RTC_HOURFORMAT12_AM,
                 uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);

    // Gets the time from the RTC in human readable format.
    RTC_TimeTypeDef getTime(uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t *_ss, uint8_t *_pmAm,
                            uint32_t *_dayLightSaving);
    // Gets the time from the RTC and returns it in STM32 Typedef format.
    RTC_TimeTypeDef getTime();

    // Sets the date inside STM32 RTC.
    void setDate(uint8_t _d, uint8_t _m, uint16_t _y, uint8_t _weekday);

    // Gets the date from the STM32 RTC in human readable format.
    RTC_DateTypeDef getDate(uint8_t *_d, uint8_t *_m, uint8_t *_y, uint8_t *_weekday);

    // Gets the date in STM32 Typedef format.
    RTC_DateTypeDef getDate();

    // Enables alarm on specific time and date with RTC.
    void enableAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                     uint8_t _pmAm = RTC_HOURFORMAT12_AM, uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);

    // Allow RTC interrupt on Alarm Event with callback.
    void enableAlarmInterrupt(void (*_f)());

    // Disable specific alarm.
    void disableAlarm(uint32_t _alarm);

    // Disable only alarm interrupt.
    void disableAlarmInterrupt();

    // Enable alarm without the Alarm Interrupt.
    void enableSimpleAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                           uint8_t _pmAm = RTC_HOURFORMAT12_AM, uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);
    
    // Check for the alarm (polling method).
    bool checkForAlarm(bool _clearFlag = true);

    // Get the previouslly set alarm time and date.
    RTC_AlarmTypeDef getAlarm(uint8_t *_d, uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t _alarm, uint32_t *_alarmMask,
                              uint8_t *_pmAm, uint32_t *_dayLightSaving);

    // Get the alarm data in STM32 RTC Typedef data type.
    RTC_AlarmTypeDef getAlarm();

    // Enable or disable output signal on GPIO from the RTC Alarm.
    void setAlarmOutput(bool _outEn, uint32_t _alarm);

    // Check if the RTC is set by reading one specific memory location from the RTC Backup RAM.
    bool isRTCSet();

    // Sets RTC is set flag by writing secret key into the RTC backup RAM.
    void rtcSetFlag();

    // Write data to the backup RAM (not the same as RTC Backup RAM!).
    void writeToBackupRAM(uint16_t _addr, void *_data, int _n);

    // Read the data from the backup RAM (not the same as RTC Backup RAM!).
    void readFromBackupRAM(uint16_t _addr, void *_data, int _n);

    // Pointer to the callback function.
    static void (*userFcn)();

  protected:

  private:
};

#endif