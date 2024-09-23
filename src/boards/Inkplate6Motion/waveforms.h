#ifndef __WAVEFROMS_INKPLATE_6_MOTION_H__

// Block usage on any other board - Arduino thing...
#ifdef BOARD_INKPLATE6_MOTION

// Include defines.h.
#include "../../system/defines.h"

// epaper panel/display general info.
#define SCREEN_WIDTH    1024ULL
#define SCREEN_HEIGHT   758ULL
#define SCREEN_MODEL_PN "ED060XC3"

// LUT for fast (and easy) pixel access and clear inside the framebuffer.
static uint8_t pixelMaskLUT[8] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};
static uint8_t pixelMaskGLUT1[2] = {0b11110000, 0b00001111};

// LUT for the 1 bit "Waveform" helpers.
// 1 Bit mode actually does not uses waveforms, but there is always a posibillity for future improvments.
static uint8_t LUTBW[16] = {0b10101010, 0b10101001, 0b10100110, 0b10100101, 0b10011010, 0b10011001, 0b10010110, 0b10010101, 0b01101010, 0b01101001, 0b01100110, 0b01100101, 0b01011010, 0b01011001, 0b01010110, 0b01010101};
static uint8_t LUTW[16] = {0b11111111, 0b11111110, 0b11111011, 0b11111010, 0b11101111, 0b11101110, 0b11101011, 0b11101010, 0b10111111, 0b10111110, 0b10111011, 0b10111010, 0b10101111, 0b10101110, 0b10101011, 0b10101010};
static uint8_t LUTB[16] = {0b11111111, 0b11111101, 0b11110111, 0b11110101, 0b11011111, 0b11011101, 0b11010111, 0b11010101, 0b01111111, 0b01111101, 0b01110111, 0b01110101, 0b01011111, 0b01011101, 0b01010111, 0b01010101};
static uint8_t LUTP[16] = {0b11111111, 0b11111100, 0b11110011, 0b11110000, 0b11001111, 0b11001100, 0b11000011, 0b11000000, 0b00111111, 0b00111100, 0b00110011, 0b00110000, 0b00001111, 0b00001100, 0b00000011, 0b00000000};
static uint8_t LUTD[16] = {0b11111111, 0b11111100, 0b11110011, 0b11110000, 0b11001111, 0b11001100, 0b11000011, 0b11000000, 0b00111111, 0b00111100, 0b00110011, 0b00110000, 0b00001111, 0b00001100, 0b00000011, 0b00000000};

static uint8_t *wavefrom1BitLUT[] = {LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTD};
static uint8_t *wavefrom1BitPartialLUT[] = {LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB, LUTB};
static uint8_t clearWavefrom1Bit[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

// Default 4 bit wavefrom.
static uint8_t waveform4BitLUT[17][16] = 
{
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 1, 1, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 0, 2, 2, 1, 1, 2},
    {1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2},
    {1, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static uint8_t waveform4BitPartialLUT[17][16] = 
{
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 1, 1, 2, 2},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 0, 2, 2, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 2},
    {1, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static uint8_t waveform4BitPartialLUTClean[11][16] = 
{
    {2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 3, 3, 1, 1, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 3, 1, 1, 1, 1, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
};

    // static uint8_t myLut[11][16] = 
    // { // >>> Color >>> Black to white                    Phase
    //   // 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15,
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 1, 0, 0, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0},
    // };

    // static uint8_t myLut2[16][16] = 
    // { // >>> Color >>> Black to white                    Phase
    //   // 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15,
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 1, 0, 0, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},
    //     {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //     {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // };

static uint8_t clearWavefrom4Bit[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

// Defines for the each display update mode.
// 4 bit full update - Global update with clean.
static InkplateWaveform default4BitWavefrom = 
{
    .mode = INKPLATE_WF_4BIT,
    .type = INKPLATE_WF_FULL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(waveform4BitLUT) / sizeof(waveform4BitLUT[0]),
    .lut = (uint8_t*)&(waveform4BitLUT[0]),
    .cycleDelay = 140ULL,
    .clearPhases = sizeof(clearWavefrom4Bit) / sizeof(clearWavefrom4Bit[0]),
    .clearLUT = clearWavefrom4Bit,
    .clearCycleDelay = 140ULL,
    .name = "default4BitFullUpdate",
};

// 1 bit full update - Global update with clean.
static InkplateWaveform default1BitWavefrom = 
{
    .mode = INKPLATE_WF_1BIT,
    .type = INKPLATE_WF_FULL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(wavefrom1BitLUT) / sizeof(wavefrom1BitLUT[0]),
    .lut = (uint8_t*)&(wavefrom1BitLUT[0]),
    .cycleDelay = 140ULL,
    .clearPhases = sizeof(clearWavefrom1Bit) / sizeof(clearWavefrom1Bit[0]),
    .clearLUT = clearWavefrom1Bit,
    .clearCycleDelay = 140ULL,
    .name = "default1BitFullUpdate",
};

// 4 bit partial update - partial update with removal of prev. pixles (fast clean display flashing).
static InkplateWaveform default4BitPartialUpdate = 
{
    .mode = INKPLATE_WF_4BIT,
    .type = INKPLATE_WF_PARTIAL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(waveform4BitLUT) / sizeof(waveform4BitLUT[0]),
    .lut = (uint8_t*)&(waveform4BitLUT[0]),
    .cycleDelay = 140ULL,
    .clearPhases = sizeof(waveform4BitPartialLUTClean) / sizeof(waveform4BitPartialLUTClean[0]),
    .clearLUT = (uint8_t*)&(waveform4BitPartialLUTClean[0]),
    .clearCycleDelay = 140ULL,
    .name = "default4BitPartialUpdate",
};

/*
Wavefrom example - Do not use this wavefrom. This only shows how the waveform array is constructed
in case if you want create one by your self. But keep in mind that you can damage your screen with bad
waveform!
Each column represents one color and each row in the color represents one phase.
There are 16 colors since the ePaper panel works with 16 bit color.
Number of phases are not limited, but keep in mind that more phases = slower refresh,
but better quality image.
static uint8_t waveform4Bit[16][16] = 
{ // >>> Color >>> Black to white                    Phase:
  // 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
    {0, 0, 0, 2, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0}, // 0
    {1, 1, 0, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 2, 1, 0}, // 1
    {1, 1, 2, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 0}, // 2
    {1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 0, 0}, // 3
    {1, 2, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 0, 0, 0, 0}, // 4
    {2, 2, 0, 2, 1, 1, 0, 1, 2, 2, 2, 2, 2, 2, 1, 0}, // 5
    {1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 0}, // 6
    {1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 0}, // 7
    {1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 0}, // 8
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0}, // 9
    {1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 0, 1, 0}, // 10
    {1, 1, 0, 1, 1, 1, 2, 2, 1, 2, 1, 1, 2, 0, 2, 0}, // 11
    {2, 1, 0, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, // 12
    {1, 1, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2}, // 13
    {1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 2, 2, 1, 1, 2, 2}, // 14
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 15
};
*/

#endif

#endif