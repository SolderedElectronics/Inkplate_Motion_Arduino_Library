/**
 **************************************************
 *
 * @file        STM32H7RTC.cpp
 * @brief       Main source file for the STM32 built-in RTC
 *              peripheral. RTC uses exteranl clock source (external XTAL
 *              of 32.768kHz). Library Sets and gets the time, date, alarms,
 *              sets user Alarm callbacks, manages backup memory region etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include main library header file.
#include "STM32H7RTC.h"

// User callback function pointer.
void (*STM32H7RTC::userFcn)() = nullptr;

/**
 * @brief   Constructor for a new STM32H7RTC object.
 * 
 */
STM32H7RTC::STM32H7RTC()
{
    // Empty constructor.
}

/**
 * @brief   Initialize the RTC inside STM32.
 * 
 * @param   uint8_t _format
 *          Clock format - RTC_HOURFORMAT_24 for 24 hour clock or
 *          RTC_HOURFORMAT_12 for 12 hour format.
 *          
 * @param   bool _resetRtc
 *          Force RTC reset (override already set RTC).
 * @return  RTC_HandleTypeDef
 *          Returns the instance of the STM32 RTC.
 */
RTC_HandleTypeDef STM32H7RTC::begin(uint8_t _format, bool _resetRtc)
{
    // Initialize alarm callback function pointer to the NULL.
    userFcn = NULL;

    // Enable backup domain (needed for RTC).
    HAL_PWR_EnableBkUpAccess();
    
    // Enable backup regulator.
    HAL_PWR_EnableBkUpReg();

    // Set highest drive for xtal (consumes little bit more power, but RTC should be more stable).
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

    // Get the current setup for oscillators.
    RCC_OscInitTypeDef _oscConfig = {0};

    // Modify part for the LSE.
    _oscConfig.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    _oscConfig.LSEState = RCC_LSE_ON;

    // Update the settings.
    if (HAL_RCC_OscConfig(&_oscConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable the exteral RTC xtal (32.768kHz).
    enableClock(LSE_CLOCK);
    RCC_PeriphCLKInitTypeDef PeriphClkInit;
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable clock to the RTC.
    __HAL_RCC_RTC_ENABLE();

    // Initialize the RTC peripheral.
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    _stm32MotionHrtc.Instance = RTC;
    _stm32MotionHrtc.Init.HourFormat = _format;
    _stm32MotionHrtc.Init.AsynchPrediv = 127;
    _stm32MotionHrtc.Init.SynchPrediv = 255;
    _stm32MotionHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    _stm32MotionHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    _stm32MotionHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    _stm32MotionHrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    if (HAL_RTC_Init(&_stm32MotionHrtc) != HAL_OK)
    {
        Error_Handler();
    }

    // Check if the RTC is already set or RTC reset is not needed.
    if (!_resetRtc && isRTCSet())
        // Return STM32 RTC Instance.
        return _stm32MotionHrtc;

    // Enable the backup domain (must be after RTC Set check, otherwise it clears the content of the backup
    // RTC RAM and backup SRAM domain).
    enableBackupDomain();

    // Write the magic key to know the RTC is set.
    HAL_RTCEx_BKUPWrite(&_stm32MotionHrtc, RTC_BKP_INDEX, 0);

    // Set the RTC time.
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&_stm32MotionHrtc, &sTime, RTC_FORMAT) != HAL_OK)
    {
        Error_Handler();
    }

    // Set the RTC date.
    sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 1;
    sDate.Year = 21;
    if (HAL_RTC_SetDate(&_stm32MotionHrtc, &sDate, RTC_FORMAT) != HAL_OK)
    {
        Error_Handler();
    }

    // Return the STM32 RTC Instance.
    return _stm32MotionHrtc;
}

/**
 * @brief   Set the time inside STM32 RTC using human readable format.
 * 
 * @param   uint8_t _h
 *          RTC Hour value.
 * @param   uint8_t _m
 *          RTC Minute value.
 * @param   uint8_t _s
 *          RTC seconds value.
 * @param   uint8_t _ss
 *          RTC Sub-Secnods value.
 * @param   uint8_t _pmAm
 *          PM/AM indicator. Use RTC_HOURFORMAT12_AM or RTC_HOURFORMAT12_PM.
 * @param   _dayLightSaving
 *          Daylight saving setting. Possible values.
 *          RTC_DAYLIGHTSAVING_SUB1H - Substract one hour.
 *          RTC_DAYLIGHTSAVING_ADD1H - Add one hour.
 *          RTC_DAYLIGHTSAVING_NONE - Daylight saving disabled.
 */
void STM32H7RTC::setTime(uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _ss, uint8_t _pmAm, uint32_t _dayLightSaving)
{
    RTC_TimeTypeDef myTime = {0};
    myTime.Hours = _h;
    myTime.Minutes = _m;
    myTime.Seconds = _s;
    myTime.SubSeconds = _ss;
    myTime.TimeFormat = _pmAm;
    myTime.DayLightSaving = _dayLightSaving;
    myTime.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(&_stm32MotionHrtc, &myTime, RTC_FORMAT);

    // Set the flag for properly set RTC.
    rtcSetFlag();
}

/**
 * @brief   Get the time from the STM32 RTC in human readable format.
 * 
 * @param   uint8_t *_h
 *          Pointer to the address where to store hours value from the STM32 RTC.
 * @param   uint8_t *_m
 *          Pointer to the address where to store minutes value from the STM32 RTC.
 * @param   uint8_t *_s
 *          Pointer to the address where to store seconds value from the STM32 RTC.
 * @param   uint8_t *_ss
 *          Pointer to the address where to store sub-seconds value from the STM32 RTC.
 * @param   uint8_t *_pmAm
 *          Pointer to the address where to store PM/AM indicator.
 * @param   uint8_t *_dayLightSaving
 *          Pointer to the address where to store status of the daylight savings.
 * @return  RTC_TimeTypeDef
 *          Return the time in RTC Typedef format.
 * 
 * @note    Since _ss, _paAm and _dayLightSaving are not commonly used, NULL can be used as parameter if these
 *          parameters are not needed.
 */
RTC_TimeTypeDef STM32H7RTC::getTime(uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t *_ss, uint8_t *_pmAm,
                                  uint32_t *_dayLightSaving)
{
    RTC_TimeTypeDef myTime = {0};
    RTC_DateTypeDef myDate = {0};
    HAL_RTC_GetTime(&_stm32MotionHrtc, &myTime, RTC_FORMAT);
    HAL_RTC_GetDate(&_stm32MotionHrtc, &myDate, RTC_FORMAT);
    *_h = myTime.Hours;
    *_m = myTime.Minutes;
    *_s = myTime.Seconds;

    // Since these are not mandatory, these can have NULL pointer if not needed.
    if (_ss != NULL)
        *_ss = myTime.SubSeconds;
    if (_pmAm != NULL)
        *_pmAm = myTime.TimeFormat;
    if (_pmAm != NULL)
        *_dayLightSaving = myTime.DayLightSaving;

    // Return also STM32 RTC Time Typedef.
    return myTime;
}
/**
 * @brief   Gets the time from the STM32 in STM32 RTC Time Typedef.
 * 
 * @return  RTC_TimeTypeDef
 *          RTC Time in STM32 RTC Time Typedef format.
 */
RTC_TimeTypeDef STM32H7RTC::getTime()
{
    // Dummy variables since we are only looking to get RTCTime_Typedef data.
    uint8_t dummy1;
    uint32_t dummy2;
    return getTime(&dummy1, &dummy1, &dummy1, &dummy2, &dummy1, &dummy2);
}

/**
 * @brief   Set the date inside STM32 RTC.
 * 
 * @param   uint8_t _d
 *          Day of the month.
 * @param   uint8_t _m
 *          Month value.
 * @param   uint16_t _y
 *          Year value.
 * @param   uint8_t _weekday
 *          Weekday value.
 */
void STM32H7RTC::setDate(uint8_t _d, uint8_t _m, uint16_t _y, uint8_t _weekday)
{
    RTC_DateTypeDef myDate = {0};
    myDate.WeekDay = _weekday;
    myDate.Month = _m;
    myDate.Date = _d;
    myDate.Year = (uint8_t)(_y % 100);
    HAL_RTC_SetDate(&_stm32MotionHrtc, &myDate, RTC_FORMAT);

    rtcSetFlag();
}

/**
 * @brief   Get the date from the STM32 RTC.
 * 
 * @param   uint8_t *_d
 *          Pointer to the address where to store the day of the month.
 * @param   uint8_t *_m
 *          Pointer to the address where to store the month value.
 * @param   uint16_t *_y
 *          Pointer to the address where to store the year value.
 * @param   uint8_t *_weekday
 *          Pointer to the address where to store the weekday value.
 * @return  RTC_DateTypeDef
 *          Returns the date in STM32 RTC DateTypeDef format.
 * 
 * @note    If _weekday is not required, NULL can be used as parameter.
 */
RTC_DateTypeDef STM32H7RTC::getDate(uint8_t *_d, uint8_t *_m, uint8_t *_y, uint8_t *_weekday)
{
    RTC_DateTypeDef myDate = {0};
    HAL_RTC_GetDate(&_stm32MotionHrtc, &myDate, RTC_FORMAT);
    *_weekday = myDate.WeekDay;
    *_m = myDate.Month;
    *_d = myDate.Date;
    *_y = myDate.Year;
    return myDate;
}

/**
 * @brief   Get the date from the STM32 RTC in STM32 RTC DateTypeDef format.
 * 
 * @return  RTC_DateTypeDef
 *          RTC Date in STM32 RTC DateTypeDef format.
 */
RTC_DateTypeDef STM32H7RTC::getDate()
{
    uint8_t dummy;
    return getDate(&dummy, &dummy, &dummy, &dummy);
}

/**
 * @brief   Enables the RTC Alarm with Interrutps.
 * 
 * @param   uint8_t _d
 *          Set the month day for the RTC alarm.
 * @param   uint8_t _h
 *          Set the hours for the RTC alarm.
 * @param   uint8_t _m
 *          Set minutes for ther RTC alarm.
 * @param   uint8_t _s
 *          Set seconds for the RTC alarm.
 * @param   uint32_t _alarm
 *          Select the alarm - RTC_ALARM_A or RTC_ALARM_B. Alarm A has full
 *          functionallity in this library, while Alarm B has limited.
 * @param   uint32_t _alarmMask
 *          Set Alarm match match.
 *          RTC_ALARMMASK_NONE - No mask is applied. This means all fields (date, hours, minutes,
 *                               and seconds) must match exactly for the alarm to trigger
 *          RTC_ALARMMASK_DATEWEEKDAY - Alarm will be triggered once every day.
 *          RTC_ALARMMASK_HOURS - Alarm will be triggered once every hour.
 *          RTC_ALARMMASK_MINUTES - Alarm will be triggered once every minute.
 *          RTC_ALARMMASK_SECONDS - Alarm will be triggered every second.
 *          RTC_ALARMMASK_ALL -  All fields (date/weekday, hours, minutes, and seconds) are ignored.
 *                               This means the alarm will always trigger
 * @param   uint8_t _pmAm
 *          PM/AM indicator (if 12 hour mode is used, otherwise ignored). Use RTC_HOURFORMAT12_AM
 *          or RTC_HOURFORMAT12_PM.
 * @param   uint32_t _dayLightSaving
 *          Set daylightime savings. Use RTC_DAYLIGHTSAVING_SUB1H, RTC_DAYLIGHTSAVING_ADD1H or
 *          RTC_DAYLIGHTSAVING_NONE 
 *          
 */
void STM32H7RTC::enableAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                           uint8_t _pmAm, uint32_t _dayLightSaving)
{
    RTC_AlarmTypeDef myAlarm = {0};
    myAlarm.AlarmTime.Hours = _h;
    myAlarm.AlarmTime.Minutes = _m;
    myAlarm.AlarmTime.Seconds = _s;
    myAlarm.AlarmTime.TimeFormat = _pmAm;
    myAlarm.AlarmTime.DayLightSaving = _dayLightSaving;
    myAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    myAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    myAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    myAlarm.AlarmDateWeekDay = _d;
    myAlarm.Alarm = _alarm;
    myAlarm.AlarmMask = _alarmMask;
    HAL_RTC_SetAlarm_IT(&_stm32MotionHrtc, &myAlarm, RTC_FORMAT);
}

/**
 * @brief   Enable callback on alarm interrupt event.
 * 
 * @param   void (*_f)() Callback to the function that will be called at the Alarm event.
 */
void STM32H7RTC::enableAlarmInterrupt(void (*_f)())
{
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    userFcn = _f;
}

/**
 * @brief   Disable prev. set alarm.
 * 
 * @param   uint32_t _alarm
 *          Alarm A or Alarm B. Use RTC_ALARM_A or RTC_ALARM_B. Alarm A has full
 *          functionallity in this library, while Alarm B has limited.
 */
void STM32H7RTC::disableAlarm(uint32_t _alarm)
{
    HAL_RTC_DeactivateAlarm(&_stm32MotionHrtc, _alarm);
}

/**
 * @brief   Disable alarm Interrupts.
 * 
 */
void STM32H7RTC::disableAlarmInterrupt()
{
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
    userFcn = NULL;
}

/**
 * @brief   Enable the simple alarm without interrupts. Must use polling method
 *          to check the state of the alarm.
 * 
 * @param   uint8_t _d
 *          Set the RTC Alarm day in month.
 * @param   uint8_t _h
 *          Set RTC Alarm hours.
 * @param   uint8_t _m
 *          Set RTC Alarm minutes.
 * @param   uint8_t _s
 *          Set RTC Alarm seconds.
 * @param   uint32_t _alarm
 *          Set RTC Alarm - Alarm A or Alarm B. Use RTC_ALARM_A or RTC_ALARM_B. Alarm A has full
 *          functionallity in this library, while Alarm B has limited.
 * @param   uint32_t _alarmMask
 *          Set Alarm match match.
 *          RTC_ALARMMASK_NONE - No mask is applied. This means all fields (date, hours, minutes,
 *                               and seconds) must match exactly for the alarm to trigger
 *          RTC_ALARMMASK_DATEWEEKDAY - Alarm will be triggered once every day.
 *          RTC_ALARMMASK_HOURS - Alarm will be triggered once every hour.
 *          RTC_ALARMMASK_MINUTES - Alarm will be triggered once every minute.
 *          RTC_ALARMMASK_SECONDS - Alarm will be triggered every second.
 *          RTC_ALARMMASK_ALL -  All fields (date/weekday, hours, minutes, and seconds) are ignored.
 *                               This means the alarm will always trigger
 * @param   uint8_t _pmAm
 *          PM/AM indicator (if 12 hour mode is used, otherwise ignored). Use RTC_HOURFORMAT12_AM
 *          or RTC_HOURFORMAT12_PM.
 * @param   uint32_t _dayLightSaving
 *          Set daylightime savings. Use RTC_DAYLIGHTSAVING_SUB1H, RTC_DAYLIGHTSAVING_ADD1H or
 *          RTC_DAYLIGHTSAVING_NONE 
 *          
 */
void STM32H7RTC::enableSimpleAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                                 uint8_t _pmAm, uint32_t _dayLightSaving)
{
    RTC_AlarmTypeDef myAlarm = {0};
    myAlarm.AlarmTime.Hours = _h;
    myAlarm.AlarmTime.Minutes = _m;
    myAlarm.AlarmTime.Seconds = _s;
    myAlarm.AlarmTime.TimeFormat = _pmAm;
    myAlarm.AlarmTime.DayLightSaving = _dayLightSaving;
    myAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    myAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    myAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    myAlarm.AlarmDateWeekDay = _d;
    myAlarm.Alarm = _alarm;
    myAlarm.AlarmMask = _alarmMask;
    HAL_RTC_SetAlarm(&_stm32MotionHrtc, &myAlarm, RTC_FORMAT);
}

/**
 * @brief   Check if the Alarm is triggered. Only for Alarm A for now.
 *          This is used along with enableSimpleAlarm().
 * 
 * @param   bool _clearFlag
 *          Set to true to clear alarm flag rightaway, set to false to keep it.
 * @return  bool
 *          true = Alarm has been triggered.
 *          false = Alarm is still not triggered.
 */
bool STM32H7RTC::checkForAlarm(bool _clearFlag)
{
    bool _r = (__HAL_RTC_ALARM_GET_FLAG(&_stm32MotionHrtc, RTC_FLAG_ALRAF)) == 0 ? false : true;
    if (_r && _clearFlag)
        __HAL_RTC_ALARM_CLEAR_FLAG(&_stm32MotionHrtc, RTC_FLAG_ALRAF);
    return _r;
}

/**
 * @brief   Get the values of previously set alarm.
 * 
 * @param   uint8_t *_d
 *          Pointer to the address where to store day in a month for the RTC Alarm.
 * @param   uint8_t *_h
 *          Pointer to the address where to store hours for the RTC Alarm.
 * @param   uint8_t *_m
 *          Pointer to the address where to store minutes for the RTC Alarm.
 * @param   uint8_t *_s
 *          Pointer to the address where to store seconds for the RTC Alarm.
 * @param   uint32_t _alarm
 *          Select the RTC Alarm - Alarm A or Alarm B. Use RTC_ALARM_A or RTC_ALARM_B. Alarm A has full
 *          functionallity in this library, while Alarm B has limited.
 * @param   uint32_t *_alarmMask
 *          Set Alarm match match.
 *          RTC_ALARMMASK_NONE - No mask is applied. This means all fields (date, hours, minutes,
 *                               and seconds) must match exactly for the alarm to trigger
 *          RTC_ALARMMASK_DATEWEEKDAY - Alarm will be triggered once every day.
 *          RTC_ALARMMASK_HOURS - Alarm will be triggered once every hour.
 *          RTC_ALARMMASK_MINUTES - Alarm will be triggered once every minute.
 *          RTC_ALARMMASK_SECONDS - Alarm will be triggered every second.
 *          RTC_ALARMMASK_ALL -  All fields (date/weekday, hours, minutes, and seconds) are ignored.
 *                               This means the alarm will always trigger
 * @param   uint8_t *_pmAm
 *          PM/AM indicator (if 12 hour mode is used, otherwise ignored). Use RTC_HOURFORMAT12_AM
 *          or RTC_HOURFORMAT12_PM.
 * @param   uint32_t *_dayLightSaving
 *          Set daylightime savings. Use RTC_DAYLIGHTSAVING_SUB1H, RTC_DAYLIGHTSAVING_ADD1H or
 *          RTC_DAYLIGHTSAVING_NONE 
 * @return  RTC_AlarmTypeDef
 *          Retrun STM32 RTC Alarm Typedef.
 *          
 */
RTC_AlarmTypeDef STM32H7RTC::getAlarm(uint8_t *_d, uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t _alarm,
                                    uint32_t *_alarmMask, uint8_t *_pmAm, uint32_t *_dayLightSaving)
{
    RTC_AlarmTypeDef myAlarm = {0};
    HAL_RTC_GetAlarm(&_stm32MotionHrtc, &myAlarm, _alarm, RTC_FORMAT);
    *_d = myAlarm.AlarmDateWeekDay;
    *_h = myAlarm.AlarmTime.Hours;
    *_m = myAlarm.AlarmTime.Minutes;
    *_s = myAlarm.AlarmTime.Seconds;
    *_alarmMask = myAlarm.AlarmMask;
    *_pmAm = myAlarm.AlarmTime.TimeFormat;
    *_dayLightSaving = myAlarm.AlarmTime.DayLightSaving;
    return myAlarm;
}

/**
 * @brief   Get the RTC Alarm Typedef filled with RTC Alarm data.
 * 
 * @return  RTC_AlarmTypeDef
 *          Retrun STM32 RTC Alarm Typedef. 
 */
RTC_AlarmTypeDef STM32H7RTC::getAlarm()
{
    uint8_t dummy1;
    uint32_t dummy2;
    return getAlarm(&dummy1, &dummy1, &dummy1, &dummy1, dummy2, &dummy2, &dummy1, &dummy2);
}

// RTC_OUTPUT_ALARMA, RTC_OUTPUT_ALARMB or RTC_OUTPUT_DISABLE for _alarm
/**
 * @brief   Route RTC Alarm signal to the GPIO pin.
 * 
 * @param   bool _outEn
 *          True - Enable the RTC Alarm Output on GPIO pin.
 *          False - Disable RTC Alarm Output on GPIO pin.
 * @param   uint32_t _alarm
 *          RTC_OUTPUT_ALARMA, RTC_OUTPUT_ALARMB or RTC_OUTPUT_DISABLE.
 * 
 * @note    If used _outEn = false, _alarm must be RTC_OUTPUT_DISABLE.
 *          
 */
void STM32H7RTC::setAlarmOutput(bool _outEn, uint32_t _alarm)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (_outEn)
    {
        // PC13 is output from RTC
        __HAL_RCC_GPIOC_CLK_ENABLE();
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        _stm32MotionHrtc.Init.OutPut = _alarm;
        // You can change impulse polarity here. By default, on alarm event output will be pulled to high.
        _stm32MotionHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        _stm32MotionHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    }
    else
    {
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
        _stm32MotionHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
        _stm32MotionHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        _stm32MotionHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    }
    if (HAL_RTC_Init(&_stm32MotionHrtc) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief   Writes a magic number to know that RTC is already set.
 * 
 */
void STM32H7RTC::rtcSetFlag()
{
    // Write a magic number to know that RTC is already set.
    HAL_RTCEx_BKUPWrite(&_stm32MotionHrtc, RTC_BKP_INDEX, RTC_BKP_VALUE);
}

/**
 * @brief   Check if the RTC is alreday set (time and date).
 * 
 * @return  bool
 *          true - RTC is already set.
 *          false - RTC is still not set. 
 */
bool STM32H7RTC::isRTCSet()
{
    return (HAL_RTCEx_BKUPRead(&_stm32MotionHrtc, RTC_BKP_INDEX) == RTC_BKP_VALUE) ? true : false;
}

/**
 * @brief   Allows to write into the backup RAM region used by the RTC.
 * 
 * @param   uint16_t _addr
 *          Start address - goes from 0 to 4095.
 * @param   void *_data
 *          Pointer to the data that needs to be written.
 * @param   _n
 *          How many bytes needs to be written.
 * 
 * @note    This is not the same memory location as RTC Backup RAM. This is SRAM Backup region.
 */
void STM32H7RTC::writeToBackupRAM(uint16_t _addr, void *_data, int _n)
{
    // Convert backup RAM address (from 0 - 4095) to the physical address of the register.
    uint8_t *_reg = (uint8_t*)(0x38800000 + (_addr & 0x00000FFF));

    // Convert data into bytes.
    uint8_t *_byteData = (uint8_t*)_data;

    // Flush the cache! Min. size for the cache flush is 32 bytes.
    SCB_CleanDCache_by_Addr((uint32_t *)_byteData, _n>32?_n:32);

    // Store bytes.
    for (int i = 0; i < _n; i++)
    {
        _reg[i] = _byteData[i];
    }

    // Flush the cache! Min. size for the cache flush is 32 bytes.
    SCB_CleanDCache_by_Addr((uint32_t *)_reg, _n>32?_n:32);
}

/**
 * @brief   Allows to read data from the backup RAM region used by the RTC.
 * 
 * @param   uint16_t _addr
 *          Start address - goes from 0 to 4095. 
 * @param   void *_data
 *          Pointer to the address where to store data from the Backup SRAM.
 * @param   int _n
 *          How many bytes needs to be written.
 * 
 * @note    This is not the same memory location as RTC Backup RAM. This is SRAM Backup region.
 */
void STM32H7RTC::readFromBackupRAM(uint16_t _addr, void *_data, int _n)
{
    // Convert backup RAM address (from 0 - 4095) to the physical address of the register.
    uint8_t *_reg = (uint8_t*)(0x38800000 + (_addr & 0x00000FFF));

    // Convert data into bytes.
    uint8_t *_byteData = (uint8_t*)_data;

    // Flush the cache! Min. size for the cache flush is 32 bytes.
    SCB_CleanDCache_by_Addr((uint32_t *)_reg, _n>32?_n:32);

    // Store bytes.
    for (int i = 0; i < _n; i++)
    {
        _byteData[i] = _reg[i];
    }

    // Flush the cache! Min. size for the cache flush is 32 bytes.
    SCB_CleanDCache_by_Addr((uint32_t *)_byteData, _n>32?_n:32);
}

/**
 * @brief   STM32 HALL Callback function - Called in the event of the Alarm Interrupt.
 * 
 * @param   RTC_HandleTypeDef *hrtc
 *          STM32 RTC Instance - needed by the STM32 HAL Library.
 */
extern "C" void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    UNUSED(hrtc);
    if (STM32H7RTC::userFcn != NULL)
        (*STM32H7RTC::userFcn)();
}

/**
 * @brief   STM32 RTC Alarm Callback registrsation.
 * 
 */
extern "C" void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&_stm32MotionHrtc);
}

/**
 * @brief   Hardware peripheral initialization.
 * 
 * @param   RTC_HandleTypeDef *hrtc
 *          STM32 RTC Instance - needed by the STM32 HAL Library.
 * 
 */
extern "C" void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        __HAL_RCC_RTC_ENABLE();
    }
}

/**
 * @brief   Hardware peripheral deinitialization.
 * 
 * @param   RTC_HandleTypeDef *hrtc
 *          STM32 RTC Instance - needed by the STM32 HAL Library.
 * 
 */
extern "C" void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        __HAL_RCC_RTC_DISABLE();
    }
}