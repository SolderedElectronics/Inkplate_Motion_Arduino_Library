// Include main header file.
#include "helpers.h"

// Include all system libraries.
#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

static enum bmpErrors _bmpError = 0;
static uint8_t *_framebuffer = NULL;
static uint16_t _framebufferW = 0;
static uint16_t _framebufferH = 0;

void setConsoleColor(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void initBmpDecoder(void *_fbPtr, uint16_t _framebufferWPx, uint16_t _framebufferHPx)
{
    // Copy parameters locally.
    _framebuffer = _fbPtr;
    _framebufferW = _framebufferWPx;
    _framebufferH = _framebufferHPx;
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
                _bmpHeaderPtr->customPalette = true;
                _retValue = true;
            }
            else
            {
                // Something is wrong if you got here.
                _bmpError = BMP_DECODE_ERR_UNVALID_COLOR_PALETTE;
            }
        }
        else
        {
            _bmpHeaderPtr->customPalette = false;
            _retValue = true;
        }
    }

    // Set back old file positon.
    fsetpos(_file, &_currentPos);

    // Return the success of processing a header (true - header was parsed correctly, false - header parsing failed).
    return _retValue;
}

bool vaildBMP(bmpHeader *_header)
{
    // Check if decoder is able to decode this file. If not return error and save error code.
    // Check for the compression. Any kind of comression is not supported.
    if (_header->infoHeader.compression != 0)
    {
        _bmpError = BMP_DECODE_ERR_COMPRESSION_NOT_SUPPORTED;
        return false;
    }

    // Check for the colors. 1, 2, 8, 16 and 24 bits are allowed.
    if (!((_header->infoHeader.bitCount == 1) || (_header->infoHeader.bitCount == 4) || (_header->infoHeader.bitCount == 8) || (_header->infoHeader.bitCount == 16) || (_header->infoHeader.bitCount == 24)))
    {
        // Save the error.
        _bmpError = BMP_DECODE_ERR_COLOR_DEPTH_NOT_SUPPORTED;
        return false;
    }

    // Check the file size.
    if (_header->infoHeader.imageSize > BMP_DECODE_MAX_MEMORY_USAGE)
    {
        // Save the error code.
        _bmpError = BMP_DECODE_ERR_NOT_ENOUGH_MEM;
        return false;
    }

    // Otherwise return ok.
    return true;
}

bool processBmp(FILE *_file, bmpHeader *_bmpHeader)
{
    // Calculate how many bytes is one line. Note that everything is aligned to the 32 bits.
    const uint32_t _widthBits = (_bmpHeader->infoHeader.width * _bmpHeader->infoHeader.bitCount);
    const uint32_t _oneLineBytes = (uint32_t)(ceil(_widthBits / 32.0)) * 4;

    // Create buffer for one line.
    uint8_t _oneLineBuffer[_oneLineBytes];

    printf("line width: bits: %lu, total bytes: %lu\r\n", _widthBits, _oneLineBytes);

    // Check if custom color palette is used.
    if (_bmpHeader->customPalette)
    {
        // Fill row by row but note that bitmap is upside down.
        for (int _y = _bmpHeader->infoHeader.height; _y >= 0; _y--)
        {
            // Storing in the temp framebuffer must be in RGB888, so conversion must be done accordingly.
            switch (_bmpHeader->infoHeader.bitCount)
            {
                case 1:
                  break;

                case 4:
                  break;

                case 8:
                  break;
            }
        }
    }
    else
    {
        // Fill row by row but note that bitmap is upside down.
        for (uint32_t _y = 0; _y < (_bmpHeader->infoHeader.height - 1); _y++)
        {
            // Fill one line of the BMP file into the framebuffer.
            // Do not forget to skip header and palette data.
            fseek(_file, (_oneLineBytes * _y) + _bmpHeader->header.dataOffset, SEEK_SET);
            fread(_oneLineBuffer, 1, _oneLineBytes, _file);

            // Flipped y axis (BMP thing).
            uint32_t _yFlipped = _bmpHeader->infoHeader.height - _y - 1;

            // Storing in the temp framebuffer must be in RGB888, so conversion must be done accordingly.
            switch (_bmpHeader->infoHeader.bitCount)
            {
                case 16:
                    for (uint32_t _x = 0; _x < (_bmpHeader->infoHeader.width); _x++)
                    {
                        // Read two bytes.
                        uint16_t _rawPixelData = (_oneLineBuffer[(_x * 2) + 1] << 8) | _oneLineBuffer[_x * 2];

                        // Extract the individual color components from 565RGB.
                        uint8_t _r = (_rawPixelData >> 11) & 0b00011111;
                        uint8_t _g = (_rawPixelData >> 5) & 0b00111111;
                        uint8_t _b = _rawPixelData & 0b00011111;
                    
                        // Upscale them into 8 bit.
                        _r = (_r << 3) | (_r >> 2);       // Expand from 5 bits to 8 bits
                        _g = (_g << 2) | (_g >> 4); // Expand from 6 bits to 8 bits
                        _b = (_b << 3) | (_b >> 2);    // Expand from 5 bits to 8 bits
                    
                        // Combine the components into an 888RGB color
                        uint32_t _rgb = ((_r << 16) | (_g << 8) | _b);

                        // Save it into framebuffer.
                        drawIntoFramebuffer(_x, _yFlipped, _rgb);
                    }
                    break;

                case 24:
                {
                    // Save it into framebuffer.
                    for (uint32_t _x = 0; _x < (_bmpHeader->infoHeader.width); _x++)
                    {
                        uint32_t _rgb = (_oneLineBuffer[(_x * 3) + 2] << 16) | (_oneLineBuffer[(_x * 3) + 1] << 8) | _oneLineBuffer[(_x * 3)];
                        drawIntoFramebuffer(_x, _yFlipped, _rgb);
                    }

                    break;
                }
            }
        }
    }

    return true;
}

enum bmpErrors errCode()
{
    // Save a copy.
    enum bmpErrors _err = _bmpError;

    // Clear the errors.
    _bmpError = BMP_DECODE_NO_ERROR;

    // Return error code.
    return _err;
}

void drawIntoFramebuffer(int _x, int _y, uint32_t _color)
{
    if ((_x >= _framebufferW) || (_x < 0) || (_y >= _framebufferH) || (_y < 0)) return;

    // Calculate the framebuffer array index
    uint32_t _fbArrayIndex = (_x + (_framebufferW * _y)) * 3;

    // Write the pixel value.
    _framebuffer[_fbArrayIndex + 2] = _color >> 16;
    _framebuffer[_fbArrayIndex + 1] = (_color >> 8) & 0xFF;
    _framebuffer[_fbArrayIndex] = _color & 0xFF;
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
    printf("Custom color palette: %s\r\n", _header->customPalette?"Yes":"No");
    if (_header->customPalette)
    {
        uint16_t _noOfColors = 1 << _header->infoHeader.bitCount;
        for (int i = 0; i < _noOfColors; i++)
        {
            printf("Color %d - R:%3d G:%3d B:%3d\r\n", i, _header->colorTable[i].red, _header->colorTable[i].green, _header->colorTable[i].blue);
        }
    }
}

void printErrorMessage(const char *format, ...)
{
    setConsoleColor(CONSOLE_COLOR_RED);
    va_list args;
    va_start(args, format);
    vprintf(format, args);  // Use vprintf to pass the va_list to printf
    va_end(args);
    setConsoleColor(CONSOLE_COLOR_WHITE);
}

void printRawFbData(uint16_t _height, bmpHeader *_bmpHeader)
{
    printf("\r\n\r\n");

    // Use temp pointer for the framebuffer array.
    uint8_t *_buffer = _framebuffer;

    for (int _y = 0; _y < _height; _y++)
    {
        for (int _x = 0; _x < _framebufferW; _x++)
        {
            printf("0x%02X%02X%02X, ", *(_buffer++), *(_buffer++), *(_buffer++));
        }
        printf("\r\n");
    }
}

bool dumpRawFbData(uint16_t _height, bmpHeader *_bmpHeader)
{
    // Create the file. If already file exists, overwritte it.
    FILE *_file;
    _file = fopen("framebufferDump.txt", "w+");

    // Use temp pointer for the framebuffer array.
    uint8_t *_buffer = _framebuffer;
    
    // Check if failed.
    if (!_file)
    {
        // Do not go any further.
        return false;
    }

    // Dump everything from the framebuffer into the file in HEX notation (FFFFFF ABCDEF 1234AF ...).
    for (int _y = 0; _y < _height; _y++)
    {
        for (int _x = 0; _x < _framebufferW; _x++)
        {
            fprintf(_file, "%02X%02X%02X ", *(_buffer++), *(_buffer++), *(_buffer++));
        }
    }

    // Close the file.
    fclose(_file);

    // Return true for succ.
    return true;
}