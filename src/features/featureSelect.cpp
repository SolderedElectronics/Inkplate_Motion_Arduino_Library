/**
 **************************************************
 *
 * @file        featureSelect.cpp
 * @brief       File mainly used to select (enable) each feature
 *              for the each Inkplate board. Yeah, it includes .cpp
 *              which is not a good practice, but it works and it's easy
 *              to manage it.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#include "featureSelect.h"
#ifdef BOARD_INKPLATE6_MOTION
// Inkplate6Motion has AS5600 magnetic rotary encoder, WS2812 LED, APDS9960 Sensor,
// LSM6DSO32 Accelerometer & Gryoscope as well as microSD card.
#include "Adafruit_NeoPixel/Adafruit_NeoPixel.cpp"
#include "RotaryEnc/AS5600.cpp"
#include "SparkFun_APDS9960/SparkFun_APDS9960.cpp"
#include "Adafruit_LSM6DS/Adafruit_LSM6DSO32.h"
#include "SparkFun_SHTC3/SparkFun_SHTC3.cpp"
#include "microSD/microSD.cpp"
#endif