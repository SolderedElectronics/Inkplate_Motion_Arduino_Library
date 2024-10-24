#ifndef __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__
#define __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include decoders.
#include "../../libs/bmpDecode/bmpDecode.h"

// Session typedef handler.
typedef struct
{
    SdFat *sdFat;
    File *file;
    InkplateImageDecodeFBHandler *frameBufferHandler;
}BmpDecoderSessionHandler;

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

#endif

#endif