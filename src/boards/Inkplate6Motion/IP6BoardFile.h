// Header guard.
#ifndef __INKPLATE6_MOTION_BOARD_SELECT_H__
#define __INKPLATE6_MOTION_BOARD_SELECT_H__

// Board select check.
#ifdef BOARD_INKPLATE6_MOTION

// Include Inkplate6Motion board header file.
#include "IPMotion6Driver.h"

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