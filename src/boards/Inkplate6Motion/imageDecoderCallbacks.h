#ifndef __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__
#define __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__

// Session typedef handler.
typedef struct
{
    SdFat *sdFat;
    File *file;
    volatile uint8_t *frameBuffer;
}bmpDecoderSessionHandler;

size_t readBytesFromSdBmp(bmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session typedef from the bmpDecoder handler.
    bmpDecoderSessionHandler *_session = (bmpDecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

    // Try to read requested number of bytes. If buffer is null, use file seek.ß
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

void drawIntoFramebuffer(bmpDecode_t *_bmpDecodeHandle, int16_t _x, int16_t _y, uint32_t _color)
{
    // Check for bounds.
    if ((_x >= 1024) || (_x < 0) || (_y >= 758) || (_y < 0))
    {
        // Add error if the image is out of bounds.
        _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_IMAGE_OUT_OF_BORDER;

        // Go out!
        return;
    }

    // Get the session typedef from the bmpDecoder handler.
    bmpDecoderSessionHandler *_session = (bmpDecoderSessionHandler*)_bmpDecodeHandle->sessionHandler;

    // Calculate the framebuffer array index
    uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

    _color = 0;

    // Write the pixel value.
    _session->frameBuffer[_fbArrayIndex + 2] = _color >> 16;
    _session->frameBuffer[_fbArrayIndex + 1] = (_color >> 8) & 0xFF;
    _session->frameBuffer[_fbArrayIndex] = _color & 0xFF;
}

#endif