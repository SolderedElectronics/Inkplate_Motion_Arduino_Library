/**
 **************************************************
 *
 * @file        IP6BoardFile.h
 * @brief       Wrrapper for the different Inkplate boards and
 *              it's classes.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Header guard.
#ifndef __INKPLATE6_MOTION_BOARD_SELECT_H__
#define __INKPLATE6_MOTION_BOARD_SELECT_H__

// Board select check.
#ifdef BOARD_INKPLATE6_MOTION

// Include Inkplate6Motion board header file.
#include "IP6MotionDriver.h"

// Include main Arduino header file.
#include <Arduino.h>

// Wrapper for different Inkplate boards.
class InkplateBoardClass : public EPDDriver
{
  public:
    InkplateBoardClass(){};
};

#endif
#endif