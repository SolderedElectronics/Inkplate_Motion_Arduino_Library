#include "STM32H7RTC.h"

STM32H7RTC::STM32H7RTC()
{
}

//-------------------Library Functions----------------------------------
RTC_HandleTypeDef STM32H7RTC::begin(uint8_t _format, bool _resetRtc)
{
    // Enable backup domain (needed for RTC).
    HAL_PWR_EnableBkUpAccess();

    // Set highest drive for xtal (consumes little bit more power, but RTC should be more stable).
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

    // Get the current setup for oscillators.
    RCC_OscInitTypeDef _oscConfig = {0};

    // Modify part for the LSE.
    _oscConfig.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    _oscConfig.LSEState = RCC_LSE_ON;

    // // Update the settings.
    if (HAL_RCC_OscConfig(&_oscConfig) != HAL_OK)
    {
        Error_Handler();
    }

    enableClock(LSE_CLOCK);
    RCC_PeriphCLKInitTypeDef PeriphClkInit;
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_RCC_RTC_ENABLE();
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = _format;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    if (!_resetRtc && isTimeSet())
        return hrtc;

    enableBackupDomain();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_INDEX, 0);
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT) != HAL_OK)
    {
        Error_Handler();
    }
    sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 1;
    sDate.Year = 21;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT) != HAL_OK)
    {
        Error_Handler();
    }
    return hrtc;
}

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
    HAL_RTC_SetTime(&hrtc, &myTime, RTC_FORMAT);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_INDEX, RTC_BKP_VALUE);
}

RTC_TimeTypeDef STM32H7RTC::getTime(uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t *_ss, uint8_t *_pmAm,
                                  uint32_t *_dayLightSaving)
{
    RTC_TimeTypeDef myTime = {0};
    RTC_DateTypeDef myDate = {0};
    HAL_RTC_GetTime(&hrtc, &myTime, RTC_FORMAT);
    HAL_RTC_GetDate(&hrtc, &myDate, RTC_FORMAT);
    *_h = myTime.Hours;
    *_m = myTime.Minutes;
    *_s = myTime.Seconds;
    if (_ss != NULL)
        *_ss = myTime.SubSeconds;
    if (_pmAm != NULL)
        *_pmAm = myTime.TimeFormat;
    if (_pmAm != NULL)
        *_dayLightSaving = myTime.DayLightSaving;
    return myTime;
}

RTC_TimeTypeDef STM32H7RTC::getTime()
{
    uint8_t dummy1;
    uint32_t dummy2;
    return getTime(&dummy1, &dummy1, &dummy1, &dummy2, &dummy1, &dummy2);
}

void STM32H7RTC::setDate(uint8_t _d, uint8_t _m, uint16_t _y, uint8_t _weekday)
{
    RTC_DateTypeDef myDate = {0};
    myDate.WeekDay = _weekday;
    myDate.Month = _m;
    myDate.Date = _d;
    myDate.Year = (uint8_t)(_y % 100);
    HAL_RTC_SetDate(&hrtc, &myDate, RTC_FORMAT);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_INDEX, RTC_BKP_VALUE);
}

RTC_DateTypeDef STM32H7RTC::getDate(uint8_t *_d, uint8_t *_m, uint8_t *_y, uint8_t *_weekday)
{
    RTC_DateTypeDef myDate = {0};
    HAL_RTC_GetDate(&hrtc, &myDate, RTC_FORMAT);
    *_weekday = myDate.WeekDay;
    *_m = myDate.Month;
    *_d = myDate.Date;
    *_y = myDate.Year;
    return myDate;
}

RTC_DateTypeDef STM32H7RTC::getDate()
{
    uint8_t dummy;
    return getDate(&dummy, &dummy, &dummy, &dummy);
}

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
    HAL_RTC_SetAlarm_IT(&hrtc, &myAlarm, RTC_FORMAT);
}

void STM32H7RTC::enableAlarmInterrupt(void (*_f)())
{
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    userFcn = _f;
}

void STM32H7RTC::disableAlarm(uint32_t _alarm)
{
    HAL_RTC_DeactivateAlarm(&hrtc, _alarm);
}

void STM32H7RTC::disableAlarmInterrupt()
{
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
    userFcn = NULL;
}

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
    HAL_RTC_SetAlarm(&hrtc, &myAlarm, RTC_FORMAT);
}

bool STM32H7RTC::checkForAlarm(bool _clearFlag)
{
    bool _r = (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_ALRAF)) == 0 ? false : true;
    if (_r && _clearFlag)
        __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    return _r;
}

RTC_AlarmTypeDef STM32H7RTC::getAlarm(uint8_t *_d, uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t _alarm,
                                    uint32_t *_alarmMask, uint8_t *_pmAm, uint32_t *_dayLightSaving)
{
    RTC_AlarmTypeDef myAlarm = {0};
    HAL_RTC_GetAlarm(&hrtc, &myAlarm, _alarm, RTC_FORMAT);
    *_d = myAlarm.AlarmDateWeekDay;
    *_h = myAlarm.AlarmTime.Hours;
    *_m = myAlarm.AlarmTime.Minutes;
    *_s = myAlarm.AlarmTime.Seconds;
    *_alarmMask = myAlarm.AlarmMask;
    *_pmAm = myAlarm.AlarmTime.TimeFormat;
    *_dayLightSaving = myAlarm.AlarmTime.DayLightSaving;
    return myAlarm;
}

RTC_AlarmTypeDef STM32H7RTC::getAlarm()
{
    uint8_t dummy1;
    uint32_t dummy2;
    return getAlarm(&dummy1, &dummy1, &dummy1, &dummy1, dummy2, &dummy2, &dummy1, &dummy2);
}

// RTC_OUTPUT_ALARMA, RTC_OUTPUT_ALARMB or RTC_OUTPUT_DISABLE for _alarm
void STM32H7RTC::setAlarmOutput(bool _outEn, uint32_t _alarm)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (_outEn)
    {
        // PC13 is output from RTC
        __HAL_RCC_GPIOC_CLK_ENABLE();
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        hrtc.Init.OutPut = _alarm;
        // You can change impulse polarity here. By default, on alarm event output will be pulled to high.
        hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    }
    else
    {
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
        hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
        hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;
    }
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }
}

bool STM32H7RTC::isTimeSet()
{
    return (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_INDEX) == RTC_BKP_VALUE) ? true : false;
}


void STM32H7RTC::writeToBackupReg(uint16_t _addr, void* _ptr, int _n)
{

}

void STM32H7RTC::readFromBackupReg(uint16_t _addr, void* _ptr, int _n)
{

}

extern "C" void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    UNUSED(hrtc);
    if (userFcn != NULL)
        (*userFcn)();
}

extern "C" void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

extern "C" void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        __HAL_RCC_RTC_ENABLE();
    }
}
extern "C" void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        __HAL_RCC_RTC_DISABLE();
    }
}
//-------------------End of Library Functions----------------------------------