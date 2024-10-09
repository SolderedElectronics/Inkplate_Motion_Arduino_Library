// Add a header guard.
#ifndef __BMPDECODE_H__
#define __BMPDECODE_H__

// Add Arduino main header file.
#include <Arduino.h>

#pragma pack(push, 1)  // Ensure byte-level packing
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
#pragma pack(pop)  // Restore the previous packing alignment

enum bmpErrors
{
    BMP_DECODE_NO_ERROR = 0,
    BMP_DECODE_ERR_UNVALID_HEADER,
    BMP_DECODE_ERR_COMPRESSION_NOT_SUPPORTED,
    BMP_DECODE_ERR_COLOR_DEPTH_NOT_SUPPORTED,
    BMP_DECODE_ERR_UNVALID_COLOR_PALETTE,
    BMP_DECODE_ERR_IMAGE_OUT_OF_BORDER
};

// BMP Decoder Class.
class BmpDecode
{
    public:
        // Class constructor.
        BmpDecode();

        // Initializer.
        void initBmpDecoder(void *_fbPtr, uint16_t _framebufferWPx, uint16_t _framebufferHPx);

        bool vaildFile(FILE *file);

        bool processHeader(FILE *_file, bmpHeader *_bmpHeaderPtr);

        bool vaildBMP(bmpHeader *_header);

        bool processBmp(FILE *_file, bmpHeader *_bmpHeader);

        enum bmpErrors errCode();

    private:
        enum bmpErrors _bmpError = BMP_DECODE_NO_ERROR;

        uint8_t *_framebuffer = NULL;

        uint16_t _framebufferW = 0;

        uint16_t _framebufferH = 0;

        void drawIntoFramebuffer(int _x, int _y, uint32_t _color);
};

#endif