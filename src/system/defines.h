/**
 **************************************************
 *
 * @file        defines.h
 * @brief       Header file Inkplate Library defines.
 *              It contains typedefs and macros for the Inkplate library usage.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#ifndef __DEFINES_H__
#define __DEFINES_H__

// Uncomment for debug messages.
// #define __INKPLATE__DEBUG__

// Debug meesage print.
#ifdef __INKPLATE__DEBUG__
#define INKPLATE_DEBUG_MGS(X)                                                                                          \
    Serial.printf("[IP DEBUG] %s\r\n", X);                                                                             \
    Serial.flush();
#else
#define INKPLATE_DEBUG_MGS(X)
#endif

// Color define macros for 1 bit mode.
#define BLACK 1
#define WHITE 0

// Different modes for the epaper.
#define INKPLATE_1BW  0
#define INKPLATE_GL16 1

// Different defines used forthe Inkplate Wavefrom typedef (see below).
#define INKPLATE_WF_1BIT           0
#define INKPLATE_WF_4BIT           1
#define INKPLATE_WF_FULL_UPDATE    0
#define INKPLATE_WF_PARTIAL_UPDATE 1

// Peripheral macros.
#define INKPLATE_ROTARY_ENCODER_PERIPH 1
#define INKPLATE_WS_LED_PERIPH         2

// Typedef structure for the Inkplate Custom Waveform.
typedef struct InkplateWaveform
{
    // INKPLATE_WF_1BIT or INKPLATE_WF_4BIT
    uint8_t mode;
    // INKPLATE_WF_FULL_UPDATE OR INKPLATE_WF_PARTIAL_UPDATE
    uint8_t type;
    // Tag to indentify waveform struct.
    uint16_t tag = 0xef;
    // Number of phases in each LUT for each color.
    uint16_t lutPhases = 0;
    // Pointer to the LUT for screen refresh.
    void *lut = NULL;
    // Number of phases for screen clear. 0 if not used.
    // In case of partial update it's used for prev image.
    uint16_t clearPhases;
    // LUT for screen clear.
    uint8_t *clearLUT;
    // Name for the wavefrom.
    const char *name;
};

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                                                            \
    {                                                                                                                  \
        int16_t t = a;                                                                                                 \
        a = b;                                                                                                         \
        b = t;                                                                                                         \
    }
#endif

#endif