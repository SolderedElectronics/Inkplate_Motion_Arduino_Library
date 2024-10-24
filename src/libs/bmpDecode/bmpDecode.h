/**
 **************************************************
 *
 * @file        bmpDecode.h
 * @brief       Header file for the Windows bitmap file
 *              decoder written in C/C++ for Inkplate
 *              Motion Boards.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/
// Add a header guard.
#ifndef __BMPDECODE_H__
#define __BMPDECODE_H__

// Add Arduino main header file.
#include <Arduino.h>

// Ensure byte-level packing
#pragma pack(push, 1)
typedef struct
{
    uint16_t signature;
    uint32_t fileSize;
    uint32_t reserved1;
    uint32_t dataOffset;
}BmpStartHeader;

typedef struct
{
    uint32_t headerSize;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    uint32_t xPixelsPerM;
    uint32_t yPixelsPerM;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
}BmpInfo;

typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved1;
}BmpColorTable;

typedef struct
{
    BmpStartHeader header;
    BmpInfo infoHeader;
    BmpColorTable colorTable[256];
    bool customPalette;
}BmpHeader;
#pragma pack(pop)
// Restore the previous packing alignment

// Error codes for BMP decoder.
enum BmpErrors
{
    BMP_DECODE_NO_ERROR = 0,
    BMP_DECODE_ERR_INVALID_HEADER,
    BMP_DECODE_ERR_COMPRESSION_NOT_SUPPORTED,
    BMP_DECODE_ERR_COLOR_DEPTH_NOT_SUPPORTED,
    BMP_DECODE_ERR_INVALID_COLOR_PALETTE,
    BMP_DECODE_ERR_IMAGE_OUT_OF_BORDER,
    BMP_DECODE_ERR_READ_FAIL
};

// Main BMP library struct typedef.
typedef struct BmpDecodeHandle BmpDecode_t;
struct BmpDecodeHandle
{
    enum BmpErrors errorCode;
    size_t (*inputFeed)(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n);
    void (*output)(void *_sessionHandler, int16_t _x, int16_t _y, uint32_t _color);
    void *sessionHandler;
    BmpHeader header;
};

bool bmpDecodeVaildFile(BmpDecode_t *_bmpDecodeHandler);

bool bmpDecodeProcessHeader(BmpDecode_t *_bmpDecodeHandler);

bool bmpDecodeVaildBMP(BmpDecode_t *_bmpDecodeHandle);

bool bmpDecodeProcessBmp(BmpDecode_t *_bmpDecodeHandle);

bool bmpDecodeGetBytes(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint16_t _n);

enum BmpErrors bmpDecodeErrCode(BmpDecode_t *_bmpDecodeHandle);

#endif