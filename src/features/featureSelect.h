#ifndef __FEATURE_SELECT_LIST_H__
#define __FEATURE_SELECT_LIST_H__

#ifdef BOARD_INKPLATE6_MOTION
#define LIBRARY_FEATURE_ROTARY_ENCODER_ENABLED
#define LIBRARY_FEATURE_WS_LED_ENABLED

#include "RotaryEnc/AS5600.h"
#include "Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#endif

#endif