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
}bmpStartHeader;

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
}bmpInfo;

typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved1;
}bmpColorTable;

typedef struct
{
    bmpStartHeader header;
    bmpInfo infoHeader;
    bmpColorTable colorTable[256];
    bool customPalette;
}bmpHeader;
#pragma pack(pop)
// Restore the previous packing alignment

// Error codes for BMP decoder.
enum bmpErrors
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
typedef struct bmpDecode_t bmpDecode_t;
struct bmpDecode_t
{
    enum bmpErrors errorCode;
    size_t (*inputFeed)(bmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n);
    void (*output)(bmpDecode_t *_bmpDecodeHandler, int16_t _x, int16_t _y, uint32_t _color);
    void *sessionHandler;
    bmpHeader header;
};

bool bmpDecodeVaildFile(bmpDecode_t *_bmpDecodeHandler);

bool bmpDecodeProcessHeader(bmpDecode_t *_bmpDecodeHandler);

bool bmpDecodeVaildBMP(bmpDecode_t *_bmpDecodeHandle);

bool bmpDecodeProcessBmp(bmpDecode_t *_bmpDecodeHandle);

bool bmpDecodeGetBytes(bmpDecode_t *_bmpDecodeHandler, void *_buffer, uint16_t _n);

enum bmpErrors bmpDecodeErrCode(bmpDecode_t *_bmpDecodeHandle);

#endif