#include "featureSelect.h"
#ifdef BOARD_INKPLATE6_MOTION
// Inkplate6Motion has AS5600 magnetic rotary encoder, WS2812 LED, APDS9960 Sensor,
// LSM6DSO32 Accelerometer & Gryoscope as well as microSD card.
#include "RotaryEnc/AS5600.cpp"
#include "Adafruit_NeoPixel/Adafruit_NeoPixel.cpp"
#include "SparkFun_SHTC3/SparkFun_SHTC3.cpp"
#include "SparkFun_APDS9960/SparkFun_APDS9960.cpp"
#include "SparkFun_LSM6DS3/SparkFunLSM6DS3.cpp"
#endif