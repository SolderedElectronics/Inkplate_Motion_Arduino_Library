#ifndef __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__
#define __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include decoders.
#include "../../libs/bmpDecode/bmpDecode.h"
#include "../../libs/TJpgDec/tjpgd.h"
#include "../../libs/pngle/pngle.h"

// Session typedef handler.
typedef struct
{
    File *file;
    InkplateImageDecodeFBHandler *frameBufferHandler;
}DecoderSessionHandler;

void static drawIntoFramebuffer(void *_framebufferHandlerPtr, int16_t _x, int16_t _y, uint32_t _color)
{
    // Convert to the InkplateImageDecodeFBHandler.
    InkplateImageDecodeFBHandler *_framebufferHandler = (InkplateImageDecodeFBHandler*)_framebufferHandlerPtr;

    // Check for bounds!
    if ((_x >= _framebufferHandler->fbWidth) || (_x < 0) || (_y >= _framebufferHandler->fbHeight) || (_y < 0)) return;

    // Calculate the framebuffer array index. Since it's RGB888 format, use multiple of three bytes.
    uint32_t _fbArrayIndex = (_x + (_framebufferHandler->fbWidth * _y)) * 3;

    // Write the pixel value.
    _framebufferHandler->framebuffer[_fbArrayIndex + 2] = _color >> 16;
    _framebufferHandler->framebuffer[_fbArrayIndex + 1] = (_color >> 8) & 0xFF;
    _framebufferHandler->framebuffer[_fbArrayIndex] = _color & 0xFF;
}

// Decoder dependant callbacks.
size_t static readBytesFromSdBmp(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session typedef from the bmpDecoder handler.
    DecoderSessionHandler *_session = (DecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

    // Try to read requested number of bytes. If buffer is null, use file seek.
    // Return value.
    size_t _retValue = 0;

    if (_buffer)
    {
        _retValue = _session->file->readBytes((uint8_t*)_buffer, _n);
    }
    else
    {
        _retValue = _session->file->seekSet(_n)?_n:0;
    }

    // Return the result.
    return _retValue;
}

void static writeBytesToFrameBufferBmp(void *_sessionHandlerPtr, int16_t _x, int16_t _y, uint32_t _color)
{
    // Decode _sessionHandler buffer into DecoderSessionHandler.
    DecoderSessionHandler *_sessionHandler = (DecoderSessionHandler*)_sessionHandlerPtr;

    // Draw pixel into the frame buffer.
    drawIntoFramebuffer(_sessionHandler->frameBufferHandler, _x, _y, _color);
}


size_t static readBytesFromSdJpg(JDEC* _jd, uint8_t* _buff, size_t _nbyte)
{
    // Session identifier (5th argument of jd_prepare function).
    DecoderSessionHandler *_sessionHandle = (DecoderSessionHandler*)_jd->device;

    // Return value.
    size_t _retValue = 0;

    // If buffer pointer is null, just set new file position.
    if (_buff)
    {
        _retValue = _sessionHandle->file->readBytes(_buff, _nbyte);
    }
    else
    {
        _retValue = _sessionHandle->file->seekCur(_nbyte)?_nbyte:0;
    }

    // Return the result.
    return _retValue;
}

/**
 * @brief   User defined output/callback function.
 * 
 * @param   JDEC* jd
 *          Decompression object.
 * @param   void* bitmap
 *          Bitmap data to be output.
 * @param   JRECT* rect
 *          Rectangle region of output image.
 * 
 * @return  Returns number of bytes read (zero on error).
 */
int static writeBytesToFrameBufferJpg(JDEC* _jd, void* _bitmap, JRECT* _rect)
{
    // Session identifier (5th argument of jd_prepare function).
    DecoderSessionHandler *_sessionHandle = (DecoderSessionHandler*)_jd->device;

    // Calculate the width and height.
    int _w = _rect->right - _rect->left + 1;
    int _h = _rect->bottom - _rect->top + 1;

    // Get the start positions.
    int _x0 = _rect->left;
    int _y0 = _rect->top;

    // Use 8 bits for the bitmap and framebuffer representation.
    uint8_t *_decodedData = (uint8_t*)(_bitmap);

    // Write the pixels into the framebuffer!
    // DMA2D could be used here?
    for (int _y = 0; _y < _h; _y++)
    {
        // Calculate source starting points for the current row.
        int srcIndex = (_w * _y);

        for (int _x = 0; _x < _w; _x++)
        {
            // Get the RGB values.
            uint8_t r = _decodedData[((srcIndex + _x) * 3) + 2];
            uint8_t g = _decodedData[((srcIndex + _x) * 3) + 1];
            uint8_t b = _decodedData[((srcIndex + _x) * 3)];

            // Write the pixel into temp. framebuffer for decoded images.
            drawIntoFramebuffer(_sessionHandle->frameBufferHandler, _x + _x0, _y + _y0, ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b));
        }
    }

    // Return 1 for success.
    return 1;
}

bool static readBytesFromSdPng(pngle_t *_pngle)
{
    // Get the session handler.
    DecoderSessionHandler *_sessionHandle = (DecoderSessionHandler*)pngle_get_user_data(_pngle);
    File *_file = _sessionHandle->file;

    uint32_t total = _file->fileSize();
    uint8_t buff[4096];
    uint32_t pnt = 0;
    int remain = 0;

    while (pnt < total)
    {
        uint32_t toread = _file->available();
        if (toread > 0)
        {
            int len = _file->read(buff, min((uint32_t)2048, toread));
            int fed = pngle_feed(_pngle, buff, len);
            if (fed < 0)
            {
                // Go back!
                return false;
            }
            remain = remain + len - fed;
            pnt += len;
        }
    }

    // If you got there, PNG is loaded successfully, return true for success.
    return true;
}

void static writeBytesToFrameBufferPng(pngle_t *_pngle, uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h, uint8_t _rgba[4])
{
    // Get the RGB values.
    uint8_t _r = _rgba[0];
    uint8_t _g = _rgba[1];
    uint8_t _b = _rgba[2];

    // Get the session handler.
    DecoderSessionHandler *_sessionHandle = (DecoderSessionHandler*)pngle_get_user_data(_pngle);

    // Write the pixel into temp. framebuffer for decoded images.
    drawIntoFramebuffer(_sessionHandle->frameBufferHandler, _x, _y, ((uint32_t)(_r) << 16) | ((uint32_t)(_g) << 8) | (uint32_t)(_b));
}

#endif

#endif