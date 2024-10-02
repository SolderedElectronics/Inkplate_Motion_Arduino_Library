// Include main header file.
#include "helpers.h"

// Include all system libraries.
#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

void setConsoleColor(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

bool vaildFile(FILE *_file)
{
    // Set default return value.
    bool _retValue = false;

    // Save the current file position.
    fpos_t _currentPos;
    fgetpos(_file, &_currentPos);

    // Rewind at the start of the file.
    rewind(_file);

    // Read 2 bytes from the file.
    uint8_t _bmpSignature[2];
    size_t _ret = fread(_bmpSignature, sizeof(uint8_t), 2, _file);

    // Check if the read was succ.
    if (_ret == 2)
    {
        // Check if this is proper BMP signature.
        if ((_bmpSignature[0] == 'B') || (_bmpSignature[1] == 'M'))
        {
            _retValue = true;
        }
    }

    // Set back old file positon.
    fsetpos(_file, &_currentPos);

    // Return error.
    return _retValue;
}

bool processHeader(FILE *_file, bmpHeader *_bmpHeaderPtr)
{
    // Set default return value.
    bool _retValue = false;

    // Save the current file position.
    fpos_t _currentPos;
    fgetpos(_file, &_currentPos);

    // Rewind at the start of the file.
    rewind(_file);

    // Read the header without the color table (maybe it does not exists).
    const uint8_t _headerSize = sizeof(bmpStartHeader) + sizeof(bmpInfo);

    // Read it!
    size_t _ret = fread(_bmpHeaderPtr, sizeof(uint8_t), _headerSize, _file);

    // Check how many bytes is read. Return fail if is diffferent than selected.
    if (_ret == _headerSize)
    {
        // Try to get color table (only available on 1, 4 or 8 bit color depth).
        if ((_bmpHeaderPtr->infoHeader.bitCount == 1) || (_bmpHeaderPtr->infoHeader.bitCount == 4) || (_bmpHeaderPtr->infoHeader.bitCount == 8))
        {
            // Copy the color data into the header.
            uint16_t _noOfColors = (1 <<_bmpHeaderPtr->infoHeader.bitCount);
            uint16_t _numberOfBytes = (1 <<_bmpHeaderPtr->infoHeader.bitCount) * sizeof(bmpColorTable);
            _ret = fread(_bmpHeaderPtr->colorTable, 1, _numberOfBytes, _file);

            // Check for success.
            if (_ret == _numberOfBytes)
            {
                _bmpHeaderPtr->customPallete = true;
                _retValue = true;
            }
        }
        else
        {
            _bmpHeaderPtr->customPallete = false;
            _retValue = true;
        }
    }

    // Set back old file positon.
    fsetpos(_file, &_currentPos);

    return _retValue;
}

bool vaildBMP(bmpHeader *_header)
{
    // Check if decoder is able to decode this file. If not return error and save error code.
    // Check for the 
}

bool processBmp(FILE *_file, bmpHeader *_bmpHeader)
{
    // Calculate how many bytes is one line. Note that everything is aligned to the 32 bits.
    uint32_t _widthBits = (_bmpHeader->infoHeader.width * _bmpHeader->infoHeader.bitCount);
    uint32_t _oneLineBytes = (uint32_t)(ceil(_widthBits / 32.0)) * 4;
    //_oneLineBytes += _oneLineBytesRem;

    // Go backwards since Windows bitmap is written from the bottom to the top.
    //for (int _h = _bmpHeader)

    printf("line width: bits: %lu, total bytes: %lu\r\n", _widthBits, _oneLineBytes);

    return true;
}

void printFileInfo(bmpHeader *_header)
{
    printf("Header Signature: 0x%04X\r\n", _header->header.signature);
    printf("File size: %lu\r\n", _header->header.fileSize);
    printf("Data offset: %lu\r\n", _header->header.dataOffset);
    printf("Info header size: %lu\r\n", _header->infoHeader.headerSize);
    printf("Image width: %lu\r\n", _header->infoHeader.width);
    printf("Image height: %lu\r\n", _header->infoHeader.height);
    printf("Number of planes: %u\r\n", _header->infoHeader.planes);
    printf("Depth: %u\r\n", _header->infoHeader.bitCount);
    printf("Compression: %lu\r\n", _header->infoHeader.compression);
    printf("Compressed Image Size: %lu\r\n", _header->infoHeader.imageSize);
    printf("X Pixels Per Meter: %lu\r\n", _header->infoHeader.xPixelsPerM);
    printf("Y Pixels Per Meter: %lu\r\n", _header->infoHeader.yPixelsPerM);
    printf("Colors Used: %lu\r\n", _header->infoHeader.colorsUsed);
    printf("Colors Important: %lu\r\n", _header->infoHeader.colorsImportant);
    printf("Custom color pallete: %s\r\n", _header->customPallete?"Yes":"No");
    if (_header->customPallete)
    {
        uint16_t _noOfColors = 1 << _header->infoHeader.bitCount;
        for (int i = 0; i < _noOfColors; i++)
        {
            printf("Color %d - R:%3d G:%3d B:%3d\r\n", i, _header->colorTable[i].red, _header->colorTable[i].green, _header->colorTable[i].blue);
        }
    }
}

void printErrorMessage(char *_msg)
{
    setConsoleColor(CONSOLE_COLOR_RED);
    printf(_msg);
    setConsoleColor(CONSOLE_COLOR_WHITE);
}