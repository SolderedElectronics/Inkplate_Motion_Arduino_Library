/**
 **************************************************
 *
 * @file        InkplateBoards.h
 * @brief       File used for the Inkplate board selection. It selects
 *              correct Inkplate Driver code as well as correct peripherals,
 *              defines etc.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#ifndef __INKPLATE_BOARDS_H__
#define __INKPLATE_BOARDS_H__

// Board selector. It only includes files for selected board.
#ifdef BOARD_INKPLATE6_MOTION
#include "../boards/Inkplate6Motion/IP6MotionDriver.h"
#endif

#endif