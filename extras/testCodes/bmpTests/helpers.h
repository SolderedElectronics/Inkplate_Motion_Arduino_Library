// Add a header guard.
#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

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
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved1;
}bmpColorTable;

typedef struct
{
    bmpStartHeader header;
    bmpInfo infoHeader;
    bmpColorTable colorTable[256];
    bool customPallete;
}bmpHeader;

#pragma pack(pop)  // Restore the previous packing alignment

#define CONSOLE_COLOR_BLACK         0 
#define CONSOLE_COLOR_BLUE          1 
#define CONSOLE_COLOR_GREEN         2 
#define CONSOLE_COLOR_AQUA          3 
#define CONSOLE_COLOR_RED           4 
#define CONSOLE_COLOR_PURPLE        5 
#define CONSOLE_COLOR_YELLOW        6 
#define CONSOLE_COLOR_WHITE         7 
#define CONSOLE_COLOR_GREY          8 
#define CONSOLE_COLOR_LIGHT_BLUE    9 
#define CONSOLE_COLOR_LIGHT_GREEN   10
#define CONSOLE_COLOR_LIGHT_AQUA    11
#define CONSOLE_COLOR_LIGHT_RED     12
#define CONSOLE_COLOR_LIGHT_PURPLE  13
#define CONSOLE_COLOR_LIGHT_YELLOW  14
#define CONSOLE_COLOR_BRIGHT_WHITE  15

void setConsoleColor(int color);
bool vaildFile(FILE *file);
bool processHeader(FILE *_file, bmpHeader *_bmpHeaderPtr);
bool vaildBMP(bmpHeader *_header);
bool processBmp(FILE *_file, bmpHeader *_bmpHeader);
void printFileInfo(bmpHeader *_header);
void printErrorMessage(char *_msg);

#endif