#ifndef __MY_TEST_LIBRARY_BOARD_SELECT_H__
#define __MY_TEST_LIBRARY_BOARD_SELECT_H__

#include <Arduino.h>

#ifdef BOARD_INKPLATE6_MOTION
#include "boards/Inkplate6Motion/IP6BoardFile.h"
//#elif defined(ARDUINO_ESP32_DEV)
//#include "boards/board2/mainBoardFile.h"
#else
#error "Board not selected!"
#endif

#endif