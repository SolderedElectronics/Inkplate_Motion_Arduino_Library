uint8_t customClearWavefrom4Bit[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};
// clang-format on

// Second thing that determ. wavefrom is the timing - how long one line write is holded. The longer, the better image
// quality, but slower refresh. This is only to some extend - too slow time can degrade image quality. This value is CPU
// cycles. There are two different timings - one for clear, other one for write. This timing is tuned to get 85Hz
// as eInk states.
uint32_t oneLineWriteClean = 550ULL;

// This is the write wavefrom - same as before; 0 = discharge, 1 = Black, 2 = White, 3 = Skip. But now this is
// two dimensional array, where each column represents one color (black to white from left to right) and rows each
// ePaper refresh phase. This is the same wavefrom as default one, but it has slower timings, so image has less noise.
// clang-format off
static uint8_t customWaveform4BitLUT[17][16] = {
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 2, 1, 0, 1, 0, 1, 1, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 2, 1, 0, 2, 0, 1, 2, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 0, 1, 1, 0},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 2, 0, 2, 0, 2, 1, 0},
    {1, 0, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 2, 1, 0},
    {1, 0, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 0},
    {1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0},
    {1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 0},
    {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0},
    {1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 0},
    {1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0},
    {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2},
    {1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
// clang-format on

// Same thing as prev. timing, only now is for writing pixels.
uint32_t oneLineWrite = 350ULL;

// Create the waveform struct.
static InkplateWaveform custom4BitWavefrom = {
    .mode = INKPLATE_WF_4BIT,
    .type = INKPLATE_WF_FULL_UPDATE,
    .tag = 0xef,
    .lutPhases = sizeof(customWaveform4BitLUT) / sizeof(customWaveform4BitLUT[0]),
    .lut = (uint8_t *)&(customWaveform4BitLUT[0]),
    .cycleDelay = oneLineWrite,
    .clearPhases = sizeof(customClearWavefrom4Bit) / sizeof(customClearWavefrom4Bit[0]),
    .clearLUT = customClearWavefrom4Bit,
    .clearCycleDelay = oneLineWriteClean,
    .name = "custom4BitFullUpdate",
};