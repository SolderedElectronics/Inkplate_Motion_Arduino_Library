// Include main header file.
#include "bmpDecode.h"

bool bmpDecodeVaildFile(bmpDecode_t *_bmpDecodeHandler)
{
    // Rewind the file at the start.
    _bmpDecodeHandler->inputFeed(_bmpDecodeHandler, NULL, 0);

    // Read 2 bytes from the file.
    if (!bmpDecodeGetBytes(_bmpDecodeHandler, &_bmpDecodeHandler->header.header.signature, 2))
    {
        // Set error flag.
        _bmpDecodeHandler->errorCode = BMP_DECODE_ERR_READ_FAIL;

        // Go back!
        return false;
    }


    // Check if this is proper BMP signature.
    if (_bmpDecodeHandler->header.header.signature != 0x4D42) // BM but in reverse.
    {
        _bmpDecodeHandler->errorCode = BMP_DECODE_ERR_INVALID_HEADER;
        return false;
    }

    // If code got here, everything went ok.
    return true;
}

bool bmpDecodeProcessHeader(bmpDecode_t *_bmpDecodeHandler)
{
    // Set default return value.
    bool _retValue = false;

    // Rewind at the start of the file.
    _bmpDecodeHandler->inputFeed(_bmpDecodeHandler, NULL, 0);

    // Read the header without the color table (maybe it does not exists).
    const uint8_t _headerSize = sizeof(bmpStartHeader) + sizeof(bmpInfo);

    // Read it!
    if (bmpDecodeGetBytes(_bmpDecodeHandler, (uint8_t*)&(_bmpDecodeHandler->header), _headerSize))
    {
        // Try to get color table (only available on 1, 4 or 8 bit color depth).
        if ((_bmpDecodeHandler->header.infoHeader.bitCount == 1) || (_bmpDecodeHandler->header.infoHeader.bitCount == 4) || (_bmpDecodeHandler->header.infoHeader.bitCount == 8))
        {
            // Copy the color data into the header.
            uint16_t _noOfColors = (1 << _bmpDecodeHandler->header.infoHeader.bitCount);
            uint16_t _numberOfBytes = (1 << _bmpDecodeHandler->header.infoHeader.bitCount) * sizeof(bmpColorTable);

            // Check for success.
            if (bmpDecodeGetBytes(_bmpDecodeHandler, _bmpDecodeHandler->header.colorTable, _numberOfBytes))
            {
                _bmpDecodeHandler->header.customPalette = true;
                _retValue = true;
            }
            else
            {
                // Something is wrong if you got here.
                _bmpDecodeHandler->errorCode = BMP_DECODE_ERR_INVALID_COLOR_PALETTE;
            }
        }
        else
        {
            _bmpDecodeHandler->header.customPalette = false;
            _retValue = true;
        }
    }
    else
    {
        _bmpDecodeHandler->errorCode = BMP_DECODE_ERR_READ_FAIL;
    }

    // Return the success of processing a header (true - header was parsed correctly, false - header parsing failed).
    return _retValue;
}

bool bmpDecodeVaildBMP(bmpDecode_t *_bmpDecodeHandle)
{
    // Check if decoder is able to decode this file. If not return error and save error code.
    // Check for the compression. Any kind of comression is not supported.
    if (_bmpDecodeHandle->header.infoHeader.compression != 0 && _bmpDecodeHandle->header.infoHeader.compression != 3)
    {
        _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_COMPRESSION_NOT_SUPPORTED;
        return false;
    }

    // Check for the colors. 1, 2, 8, 16 and 24 bits are allowed.
    if (!((_bmpDecodeHandle->header.infoHeader.bitCount == 1) || (_bmpDecodeHandle->header.infoHeader.bitCount == 4) || (_bmpDecodeHandle->header.infoHeader.bitCount == 8) || (_bmpDecodeHandle->header.infoHeader.bitCount == 16) || (_bmpDecodeHandle->header.infoHeader.bitCount == 24)))
    {
        // Save the error.
        _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_COLOR_DEPTH_NOT_SUPPORTED;
        return false;
    }

    // Otherwise return ok.
    return true;
}

bool bmpDecodeProcessBmp(bmpDecode_t *_bmpDecodeHandle)
{
    // Calculate how many bytes is one line. Note that everything is aligned to the 32 bits.
    const uint32_t _widthBits = (_bmpDecodeHandle->header.infoHeader.width * _bmpDecodeHandle->header.infoHeader.bitCount);
    const uint32_t _oneLineBytes = (uint32_t)(ceil(_widthBits / 32.0)) * 4;

    // Create buffer for one line.
    uint8_t _oneLineBuffer[_oneLineBytes];

    // Check if custom color palette is used.
    if (_bmpDecodeHandle->header.customPalette)
    {
        // Fill row by row but note that bitmap is upside down.
        for (int _y = _bmpDecodeHandle->header.infoHeader.height; _y >= 0; _y--)
        {
            // Fill one line of the BMP file into the framebuffer.
            // Do not forget to skip header and palette data.
            //fseek(_file, (_oneLineBytes * _y) + _bmpHeader->header.dataOffset, SEEK_SET);
            //size_t _retSize = fread(_oneLineBuffer, 1, _oneLineBytes, _file);

            
            _bmpDecodeHandle->inputFeed(_bmpDecodeHandle, NULL, (_oneLineBytes * _y) + _bmpDecodeHandle->header.header.dataOffset);
            if (!bmpDecodeGetBytes(_bmpDecodeHandle, _oneLineBuffer, _oneLineBytes))
            {
                // Can't read the data? Return fail with error code.
                _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_READ_FAIL;
                return true;
            }

            // Flipped y axis (BMP thing).
            uint32_t _yFlipped = _bmpDecodeHandle->header.infoHeader.height - _y - 1;

            // Storing in the temp framebuffer must be in RGB888, so conversion must be done accordingly.
            switch (_bmpDecodeHandle->header.infoHeader.bitCount)
            {
                case 1:
                {
                    // Due 8 pixels per byte packing, check how many whole bytes there are.
                    uint16_t _completeBytes = _bmpDecodeHandle->header.infoHeader.width / 8;
                    uint8_t _rem = _bmpDecodeHandle->header.infoHeader.width % 8;

                    for (int _x = 0; _x < _completeBytes; _x++)
                    {
                        // Get those 8 pixels.
                        uint8_t _px = _oneLineBuffer[_x];

                        // Write those 8 pixels.
                        for (int i = 0; i < 8; i++)
                        {
                            // Convert byte into color.
                            uint8_t _index = 7 - i;
                            uint8_t _r = _bmpDecodeHandle->header.colorTable[(_px >> _index) & 1].red;
                            uint8_t _g = _bmpDecodeHandle->header.colorTable[(_px >> _index) & 1].green;
                            uint8_t _b = _bmpDecodeHandle->header.colorTable[(_px >> _index) & 1].blue;

                            // Draw the image using converter RGB values.
                            //drawIntoFramebuffer((_x * 8) + i, _yFlipped, (_r << 16) | (_g << 8) | _b);
                            _bmpDecodeHandle->output(_bmpDecodeHandle, (_x * 8) + i, _yFlipped, (_r << 16) | (_g << 8) | _b);
                        }
                    }

                    // Check for reminder.
                    if (_rem != 0)
                    {
                        // Get those 8 pixels.
                        uint8_t _px = _oneLineBuffer[_completeBytes];

                        // Write those 8 pixels.
                        for (int i = 7; i >= (7 - _rem); i--)
                        {
                            // Convert byte into color.
                            uint8_t _index = 7 - i;
                            uint8_t _r = _bmpDecodeHandle->header.colorTable[(_px >> i) & 1].red;
                            uint8_t _g = _bmpDecodeHandle->header.colorTable[(_px >> i) & 1].green;
                            uint8_t _b = _bmpDecodeHandle->header.colorTable[(_px >> i) & 1].blue;

                            // Draw the image using converter RGB values.
                            //drawIntoFramebuffer((_completeBytes * 8) + _index, _yFlipped, (_r << 16) | (_g << 8) | _b);
                            _bmpDecodeHandle->output(_bmpDecodeHandle, (_completeBytes * 8) + _index, _yFlipped, (_r << 16) | (_g << 8) | _b);
                        }
                    }
                }
                break;

                case 4:
                {
                    // Due 2 pixels per byte packing, check how many whole bytes there are.
                    uint16_t _completeBytes = _bmpDecodeHandle->header.infoHeader.width / 2;
                    uint8_t _halfBytes = _bmpDecodeHandle->header.infoHeader.width % 2;

                    for (int _x = 0; _x < _completeBytes; _x++)
                    {
                        // Get those two pixels.
                        uint8_t _px = _oneLineBuffer[_x];
                        uint8_t _pxSplit[2] = {_px >> 4, _px & 0x0F};

                        // Write those two pixels
                        for (int i = 0; i < 2; i++)
                        {
                            // Convert byte into color.
                            uint8_t _r = _bmpDecodeHandle->header.colorTable[_pxSplit[i]].red;
                            uint8_t _g = _bmpDecodeHandle->header.colorTable[_pxSplit[i]].green;
                            uint8_t _b = _bmpDecodeHandle->header.colorTable[_pxSplit[i]].blue;

                            // Draw the image using converter RGB values.
                            //drawIntoFramebuffer((_x * 2) + i, _yFlipped, (_r << 16) | (_g << 8) | _b);
                            _bmpDecodeHandle->output(_bmpDecodeHandle, (_x * 2) + i, _yFlipped, (_r << 16) | (_g << 8) | _b);
                        }
                    }

                    // Check for reminder (half bytes).
                    if (_halfBytes != 0)
                    {
                        uint8_t _px = _oneLineBuffer[_completeBytes] >> 4;
                        uint8_t _r = _bmpDecodeHandle->header.colorTable[_px].red;
                        uint8_t _g = _bmpDecodeHandle->header.colorTable[_px].green;
                        uint8_t _b = _bmpDecodeHandle->header.colorTable[_px].blue;

                        // Draw the image using converter RGB values.
                        //drawIntoFramebuffer((_completeBytes * 2) + 1, _yFlipped, (_r << 16) | (_g << 8) | _b);
                        _bmpDecodeHandle->output(_bmpDecodeHandle, (_completeBytes * 2) + 1, _yFlipped, (_r << 16) | (_g << 8) | _b);
                    }
                }
                break;

                case 8:
                    for (int _x = 0; _x < (_bmpDecodeHandle->header.infoHeader.width); _x++)
                    {
                        // Convert byte into color.
                        uint8_t _r = _bmpDecodeHandle->header.colorTable[_oneLineBuffer[_x]].red;
                        uint8_t _g = _bmpDecodeHandle->header.colorTable[_oneLineBuffer[_x]].green;
                        uint8_t _b = _bmpDecodeHandle->header.colorTable[_oneLineBuffer[_x]].blue;

                        // Draw the image using converter RGB values.
                        //drawIntoFramebuffer(_x, _yFlipped, (_r << 16) | (_g << 8) | _b);
                        _bmpDecodeHandle->output(_bmpDecodeHandle, _x, _yFlipped, (_r << 16) | (_g << 8) | _b);
                    }
                  break;
            }
        }
    }
    else
    {
        // Fill row by row but note that bitmap is upside down.
        for (uint32_t _y = 0; _y < (_bmpDecodeHandle->header.infoHeader.height - 1); _y++)
        {
            // Fill one line of the BMP file into the framebuffer.
            // Do not forget to skip header and palette data.
            //fseek(_file, (_oneLineBytes * _y) + _bmpHeader->header.dataOffset, SEEK_SET);
            // size_t _retSize = fread(_oneLineBuffer, 1, _oneLineBytes, _file);

            _bmpDecodeHandle->inputFeed(_bmpDecodeHandle, NULL, (_oneLineBytes * _y) + _bmpDecodeHandle->header.header.dataOffset);
            if (!bmpDecodeGetBytes(_bmpDecodeHandle, _oneLineBuffer, _oneLineBytes))
            {
                // Can't read the data? Return fail with error code.
                _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_READ_FAIL;
                return true;
            }

            // Flipped y axis (BMP thing).
            uint32_t _yFlipped = _bmpDecodeHandle->header.infoHeader.height - _y - 1;

            // Storing in the temp framebuffer must be in RGB888, so conversion must be done accordingly.
            switch (_bmpDecodeHandle->header.infoHeader.bitCount)
            {
                case 16:
                    for (uint32_t _x = 0; _x < (_bmpDecodeHandle->header.infoHeader.width); _x++)
                    {
                        // Read two bytes.
                        uint16_t _rawPixelData = (_oneLineBuffer[(_x * 2) + 1] << 8) | _oneLineBuffer[_x * 2];

                        // Extract the individual color components from 565RGB.
                        uint8_t _r = (_rawPixelData >> 11) & 0b00011111;
                        uint8_t _g = (_rawPixelData >> 5) & 0b00111111;
                        uint8_t _b = _rawPixelData & 0b00011111;
                    
                        // Upscale them into 8 bit.
                        _r = (_r << 3) | (_r >> 2);
                        _g = (_g << 2) | (_g >> 4);
                        _b = (_b << 3) | (_b >> 2);
                    
                        // Combine the components into an RGB888 color.
                        uint32_t _rgb = ((_r << 16) | (_g << 8) | _b);

                        // Save it into framebuffer.
                        _bmpDecodeHandle->output(_bmpDecodeHandle, _x, _yFlipped, _rgb);
                    }
                    break;

                case 24:
                {
                    // Save it into framebuffer.
                    for (uint32_t _x = 0; _x < (_bmpDecodeHandle->header.infoHeader.width); _x++)
                    {
                        uint32_t _rgb = (_oneLineBuffer[(_x * 3) + 2] << 16) | (_oneLineBuffer[(_x * 3) + 1] << 8) | _oneLineBuffer[(_x * 3)];
                        //drawIntoFramebuffer(_x, _yFlipped, _rgb);
                        _bmpDecodeHandle->output(_bmpDecodeHandle, _x, _yFlipped, _rgb);
                    }

                    break;
                }

                case 32:
                {
                    // Implemented in the near future?
                }
                break;
            }
        }
    }

    return true;
}

enum bmpErrors bmpDecodeErrCode(bmpDecode_t *_bmpDecodeHandle)
{
    // Save a copy.
    enum bmpErrors _err = _bmpDecodeHandle->errorCode;

    // Clear the errors.
    _bmpDecodeHandle->errorCode = BMP_DECODE_NO_ERROR;

    // Return error code.
    return _err;
}

bool bmpDecodeGetBytes(bmpDecode_t *_bmpDecodeHandler, void *_buffer, uint16_t _n)
{
    uint32_t _gotBytes = 0;
    
    // Loop until you get all the bytes requested.
    while (_gotBytes != _n)
    {
        // Read the bytes and save the return code.
        // retValue = 0 -> Can't read the bytes.
        // retValue >0 -> Number of bytes read.

        uint32_t _retValue = _bmpDecodeHandler->inputFeed(_bmpDecodeHandler, _buffer, _n - _gotBytes);
        
        // Check if read has failed.
        if (!_retValue)
        {
            // Can't read anymore? Return error!
            return false;
        }
        else
        {
            // Update the variables.
            _gotBytes += _retValue;
        }
    }

    // All bytes read successfully? Return true.
    return true;
}