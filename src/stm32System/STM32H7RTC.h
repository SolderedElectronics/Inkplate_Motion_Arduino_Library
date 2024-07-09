#ifndef __STM32H7RTC_H__
#define __STM32H7RTC_H__

#include "Arduino.h"

#define RTC_FORMAT RTC_FORMAT_BIN
#define RTC_BCKP_CHCK

static void (*userFcn)() = NULL;

extern "C" void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);
extern "C" void RTC_Alarm_IRQHandler(void);
extern "C" void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc);
extern "C" void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc);
static RTC_HandleTypeDef hrtc = {0};

class STM32H7RTC
{
  public:
    STM32H7RTC();
    RTC_HandleTypeDef begin(uint8_t _format, bool _resetRtc = false);
    void setTime(uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _ss, uint8_t _pmAm = RTC_HOURFORMAT12_AM,
                 uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);
    RTC_TimeTypeDef getTime(uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t *_ss, uint8_t *_pmAm,
                            uint32_t *_dayLightSaving);
    RTC_TimeTypeDef getTime();
    void setDate(uint8_t _d, uint8_t _m, uint16_t _y, uint8_t _weekday);
    RTC_DateTypeDef getDate(uint8_t *_d, uint8_t *_m, uint8_t *_y, uint8_t *_weekday);
    RTC_DateTypeDef getDate();
    void enableAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                     uint8_t _pmAm = RTC_HOURFORMAT12_AM, uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);
    void enableAlarmInterrupt(void (*_f)());
    void disableAlarm(uint32_t _alarm);
    void disableAlarmInterrupt();
    void enableSimpleAlarm(uint8_t _d, uint8_t _h, uint8_t _m, uint8_t _s, uint32_t _alarm, uint32_t _alarmMask,
                           uint8_t _pmAm = RTC_HOURFORMAT12_AM, uint32_t _dayLightSaving = RTC_DAYLIGHTSAVING_NONE);
    bool checkForAlarm(bool _clearFlag = true);
    RTC_AlarmTypeDef getAlarm(uint8_t *_d, uint8_t *_h, uint8_t *_m, uint8_t *_s, uint32_t _alarm, uint32_t *_alarmMask,
                              uint8_t *_pmAm, uint32_t *_dayLightSaving);
    RTC_AlarmTypeDef getAlarm();
    void setAlarmOutput(bool _outEn, uint32_t _alarm);
    bool isTimeSet();
    void writeToBackupReg(uint16_t _addr, void* _ptr, int _n);
    void readFromBackupReg(uint16_t _addr, void* _ptr, int _n);
};

#endif