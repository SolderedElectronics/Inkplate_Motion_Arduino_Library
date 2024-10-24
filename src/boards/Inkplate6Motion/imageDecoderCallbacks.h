#ifndef __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__
#define __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include decoders.
#include "../../libs/bmpDecode/bmpDecode.h"
#include "../../libs/TJpgDec/tjpgd.h"

// Session typedef handler.
typedef struct
{
    File *file;
    InkplateImageDecodeFBHandler *frameBufferHandler;
}BmpDecoderSessionHandler;

// Session identifier for input/output functions for the JPG decoder.
typedef struct
{
    File *fp;
    InkplateImageDecodeFBHandler *frameBufferHandler;
}JdecIODev;

void drawIntoFramebuffer(void *_framebufferHandlerPtr, int16_t _x, int16_t _y, uint32_t _color)
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
size_t readBytesFromSdBmp(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session typedef from the bmpDecoder handler.
    BmpDecoderSessionHandler *_session = (BmpDecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

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

void writeBytesToFrameBufferBmp(void *_sessionHandlerPtr, int16_t _x, int16_t _y, uint32_t _color)
{
    // Decode _sessionHandler buffer into BmpDecoderSessionHandler.
    BmpDecoderSessionHandler *_sessionHandler = (BmpDecoderSessionHandler*)_sessionHandlerPtr;

    // Draw pixel into the frame buffer.
    drawIntoFramebuffer(_sessionHandler->frameBufferHandler, _x, _y, _color);
}


size_t readBytesFromSdJpg(JDEC* _jd, uint8_t* _buff, size_t _nbyte)
{
    // Session identifier (5th argument of jd_prepare function).
    JdecIODev *_dev = (JdecIODev*)_jd->device;

    // Return value.
    size_t _retValue = 0;

    // If buffer pointer is null, just set new file position.
    if (_buff)
    {
        _retValue = _dev->fp->readBytes(_buff, _nbyte);
    }
    else
    {
        _retValue = _dev->fp->seekCur(_nbyte)?_nbyte:0;
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
int writeBytesToFrameBufferJpg(JDEC* _jd, void* _bitmap, JRECT* _rect)
{
    // Session identifier (5th argument of jd_prepare function).
    JdecIODev *_dev = (JdecIODev*)_jd->device;

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
            drawIntoFramebuffer(_dev->frameBufferHandler, _x + _x0, _y + _y0, ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b));
        }
    }

    // Return 1 for success.
    return 1;
}
#endif

#endif