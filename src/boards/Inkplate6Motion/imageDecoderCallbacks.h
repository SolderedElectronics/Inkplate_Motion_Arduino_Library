#ifndef __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__
#define __INKPLATE_MOTION_IMAGE_DECODER_CALLBACKS_H__

// Block usage on other boards since these decoder callbacks are board specific.
#ifdef BOARD_INKPLATE6_MOTION

// Include decoders.
#include "../../libs/bmpDecode/bmpDecode.h"
#include "../../libs/TJpgDec/tjpgd.h"
#include "../../libs/pngle/pngle.h"

/**
 * @brief   Session handler. Used by image decoder to be able to pass buffers, file,
 *          framebuffer etc into the decoder callbacks.
 * 
 */
typedef struct
{
    volatile uint8_t* fileBuffer;
    size_t fileBufferSize;
    size_t bufferOffset;
    File *file;
    InkplateImageDecodeFBHandler *frameBufferHandler;
}InkplateDecoderSessionHandler;

/**
 * @brief   Function handles writing pixels into the temp. framebuffer fro decoded image.
 * 
 * @param   void *_framebufferHandlerPtr
 *          Pointer to the framebuffer handler. To be compatible with all decoders, it must
 *          be void.
 * @param   int16_t _x
 *          X position of the current pixel.
 * @param   int16_t _y
 *          X position of the current pixel.
 * @param   uint32_t _color
 *          Pixel color (must be RGB888).
 */
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
/**
 * @brief   Callback for the BMP decoder for feeding data into the decoder.
 * 
 * @param   BmpDecode_t *_bmpDecodeHandler
 *          BMP Decoder specific handler. Must not be null!
 * @param   void *_buffer
 *          Buffer where to store bytes read from the microSD card. If null, seek
 *          will be execuder by the offset defined in _n.
 * @param   uint64_t _n
 *          Number of bytes read from the microSD card (if _buffer != null), file position
 *          offset of _buffer == null.
 * @return  size_t
 *          Number of bytes succesfully read from the microSD card (if _buffer != null),
 *          0 or 1 for the seek position (_buffer == null).
 */
size_t static readBytesFromSdBmp(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session typedef from the bmpDecoder handler.
    InkplateDecoderSessionHandler *_session = (InkplateDecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

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

/**
 * @brief   Callback for the BMP decoder to write the decoded data in the temp. framebuffer for decoded image.
 * 
 * @param   void *_sessionHandlerPtr
 *          Session handler. Used by image decoder to be able to pass buffers, file,
 *          framebuffer etc into the decoder callbacks.
 *          
 * @param   int16_t _x
 *          X position of decoded pixel.
 * @param   int16_t _y
 *          Y position of decoded pixel.
 * @param   int16_t _color
 *          Color of the pixel (must be RGB888).
 *          
 */
void static writeBytesToFrameBufferBmp(void *_sessionHandlerPtr, int16_t _x, int16_t _y, uint32_t _color)
{
    // Decode _sessionHandler buffer into DecoderSessionHandler.
    InkplateDecoderSessionHandler *_sessionHandler = (InkplateDecoderSessionHandler*)_sessionHandlerPtr;

    // Draw pixel into the frame buffer.
    drawIntoFramebuffer(_sessionHandler->frameBufferHandler, _x, _y, _color);
}

/**
 * @brief   Callback for the JPG decoder for feeding data into the decoder.
 * 
 * @param   JDEC *_jpgDecoder
 *          JPG Decoder specific handler. Must not be null!
 * @param   void *_buffer
 *          Buffer where to store bytes read from the microSD card. If null, seek
 *          will be execuder by the offset defined in _n.
 * @param   uint64_t _n
 *          Number of bytes read from the microSD card (if _buffer != null), file position
 *          offset of _buffer == null.
 * @return  size_t
 *          Number of bytes succesfully read from the microSD card (if _buffer != null),
 *          0 or 1 for the seek position (_buffer == null).
 */
size_t static readBytesFromSdJpg(JDEC* _jpgDecoder, uint8_t* _buff, size_t _nbyte)
{
    // Session identifier (5th argument of jd_prepare function).
    InkplateDecoderSessionHandler *_sessionHandle = (InkplateDecoderSessionHandler*)_jpgDecoder->device;

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
 *          JPG Decoder specific handler. Must not be null!
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
    InkplateDecoderSessionHandler *_sessionHandle = (InkplateDecoderSessionHandler*)_jd->device;

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

/**
 * @brief   Callback for the PNG decoder for feeding data into the decoder.
 * 
 * @param   pngle_t *_pngle
 *          PNG Decoder specific handler. Must not be null!
 * @return  bool
 *          true - Image loaded into decoder succesfully.
 *          false - Image load failed.
 */
bool static readBytesFromSdPng(pngle_t *_pngle)
{
    // Get the session handler.
    InkplateDecoderSessionHandler *_sessionHandle = (InkplateDecoderSessionHandler*)pngle_get_user_data(_pngle);

    // Get the microSD card file pointer.
    File *_file = _sessionHandle->file;

    // Get the file size.
    uint32_t total = _file->fileSize();

    // 4k buffer for the image chunk load.
    uint8_t buff[4096];
    // Buffer and feed helper variables.
    uint32_t pnt = 0;
    int remain = 0;

    // Feed the decoder until there is no more to load.
    while (pnt < total)
    {
        uint32_t toread = _file->available();
        if (toread > 0)
        {
            int len = _file->read(buff, min((uint32_t)2048, toread));
            int fed = pngle_feed(_pngle, buff, len);

            if (fed < 0)
            {
                // Ooops, this is not good, go back return false for fail.
                return false;
            }

            // Otherwise, keep reading and feeding the decoder.
            remain = remain + len - fed;
            pnt += len;
        }
    }

    // If you got there, PNG is loaded successfully, return true for success.
    return true;
}

/**
 * @brief   Callback for the PNG decoder to write the decoded data in the temp. framebuffer for decoded image.
 * 
 * @param   pngle_t *_pngle
 *          PNG Decoder specific handler. Must not be null!
 * @param   uint32_t _x
 *          X position of the current pixel.
 * @param   uint32_t _y
 *          Y position of the current pixel.
 * @param   uint32_t _w
 *          Width of the array - not used here, just for compatibility reasons.
 * @param   uint32_t _h
 *          Height of the array - not used here, just for compatibility reasons.
 * @param   uint8_t _rgba[4]
 *          Pixel color in ARGB format (alpha layer not used here).
 *          
 */
void static writeBytesToFrameBufferPng(pngle_t *_pngle, uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h, uint8_t _rgba[4])
{
    // Get the RGB values.
    uint8_t _r = _rgba[0];
    uint8_t _g = _rgba[1];
    uint8_t _b = _rgba[2];

    // Get the session handler.
    InkplateDecoderSessionHandler *_sessionHandle = (InkplateDecoderSessionHandler*)pngle_get_user_data(_pngle);

    // Write the pixel into temp. framebuffer for decoded images.
    drawIntoFramebuffer(_sessionHandle->frameBufferHandler, _x, _y, ((uint32_t)(_r) << 16) | ((uint32_t)(_g) << 8) | (uint32_t)(_b));
}

/**
 * @brief   Function reads bytes from the SRAM or SDRAM and feeds into the BMP decoder.
 * 
 * @param   BmpDecode_t *_bmpDecodeHandler
 *          BMP Decoder specific handler. Must not be null!
 * @param   void *_buffer
 *          Buffer where to store bytes read from the microSD card. If null, seek
 *          will be execuder by the offset defined in _n.
 * @param   uint64_t _n
 *          Number of bytes read from the microSD card (if _buffer != null), file position
 *          offset of _buffer == null.
 * @return  size_t
 *          Number of bytes succesfully read from the microSD card (if _buffer != null),
 *          0 or 1 for the seek position (_buffer == null).
 */
size_t static readBytesFromBufferBmp(BmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session handler.
    InkplateDecoderSessionHandler *_session = (InkplateDecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

}

/**
 * @brief   Function reads bytes from the SRAM or SDRAM and feeds into the JPG decoder.
 * 
 * @param   JDEC *_jpgDecoder
 *          JPG Decoder specific handler. Must not be null!
 * @param   void *_buffer
 *          Buffer where to store bytes read from the microSD card. If null, seek
 *          will be execuder by the offset defined in _n.
 * @param   uint64_t _n
 *          Number of bytes read from the microSD card (if _buffer != null), file position
 *          offset of _buffer == null.
 * @return  size_t
 *          Number of bytes succesfully read from the microSD card (if _buffer != null),
 *          0 or 1 for the seek position (_buffer == null).
 */
size_t static readBytesFromBufferJpg(JDEC* _jd, uint8_t* _buff, size_t _nbyte)
{

}

/**
 * @brief   Function reads bytes from the SRAM or SDRAM and feeds into the PNG decoder.
 * 
 * @param   pngle_t *_pngle
 *          PNG Decoder specific handler. Must not be null!
 * @return  bool
 *          true - Image loaded into decoder succesfully.
 *          false - Image load failed.
 */
bool static readBytesFromBufferPng(pngle_t *_pngle)
{

}
#endif

#endif