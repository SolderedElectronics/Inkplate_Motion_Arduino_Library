/**
 **************************************************
 *
 * @file        featureSelect.h
 * @brief       File mainly used to select (enable) each feature
 *              for the each Inkplate board. Add defines macros
 *              to enable (include) each feature to the board.
 *              See the example for the Inkplate 6 Motion below.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Header guard for the feature select file.
#ifndef __FEATURE_SELECT_LIST_H__
#define __FEATURE_SELECT_LIST_H__

// Define here all features available on the Inkplate 6 Motion board.
#ifdef BOARD_INKPLATE6_MOTION

// Enable the library include for the each feature. These should not
// be changed since they are defined in each feature library.
#define LIBRARY_FEATURE_ROTARY_ENCODER_ENABLED
#define LIBRARY_FEATURE_WS_LED_ENABLED
#define LIBRARY_FEATURE_SHTC3_ENABLED
#define LIBRARY_FEATURE_APDS9960_ENABLED
#define LIBRARY_FEATURE_LSM6DSO32_ENABLED
#define LIBRARY_FEATUTE_MICROSD_ENABLED

// Peripheral names.
#define INKPLATE_PERIPHERAL_SDRAM          (1ULL << 0)
#define INKPLATE_PERIPHERAL_ROTARY_ENCODER (1ULL << 1)
#define INKPLATE_PERIPHERAL_WS_LED         (1ULL << 2)
#define INKPLATE_PERIPHERAL_SHTC3          (1ULL << 3)
#define INKPLATE_PERIPHERAL_APDS9960       (1ULL << 4)
#define INKPLATE_PERIPHERAL_LSM6DSO32        (1ULL << 5)
#define INKPLATE_PERIPHERAL_MICROSD        (1ULL << 6)
#define INKPLATE_PERIPHERAL_WIFI           (1ULL << 7)
#define INKPLATE_PERIPHERAL_ALL_PERI                                                                                   \
    (INKPLATE_PERIPHERAL_ROTARY_ENCODER | INKPLATE_PERIPHERAL_WS_LED | INKPLATE_PERIPHERAL_SHTC3 |                     \
     INKPLATE_PERIPHERAL_APDS9960 | INKPLATE_PERIPHERAL_LSM6DSO32)
#define INKPLATE_PERIPHERAL_ALL (INKPLATE_PERIPHERAL_ALL_PERI | INKPLATE_PERIPHERAL_SDRAM | INKPLATE_PERIPHERAL_WIFI)

// LIBRARY_FEATURE_ROTARY_ENCODER_ENABLED feature.
#include "RotaryEnc/AS5600.h"

// LIBRARY_FEATURE_WS_LED_ENABLED feature.
#include "Adafruit_NeoPixel/Adafruit_NeoPixel.h"

// LIBRARY_FEATURE_SHTC3_ENABLED feature.
#include "SparkFun_SHTC3/SparkFun_SHTC3.h"

// LIBRARY_FEATURE_APDS9960_ENABLED feature.
#include "SparkFun_APDS9960/SparkFun_APDS9960.h"

// LIBRARY_FEATURE_LSM6DSO_ENABLED feature.
#include "Adafruit_LSM6DS/Adafruit_LSM6DSO32.h"

// LIBRARY_FEATUTE_MICROSD_ENABLED feature.
#include "SdFat/SdFat.h"
#include "microSD/microSD.h"
#endif

#endif