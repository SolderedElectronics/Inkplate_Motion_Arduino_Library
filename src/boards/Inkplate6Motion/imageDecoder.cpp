// Include library header file.
#include "imageDecoder.h"

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include code for all image decoder callbacks.
#include "imageDecoderCallbacks.h"

// It needs also main Inkplate Motion file to get the base class.
#include "../../InkplateMotion.h"

/**
 * @brief Construct a new Image Decoder:: Image Decoder object
 * 
 */
ImageDecoder::ImageDecoder()
{
    // Empty for now.
}

void ImageDecoder::begin(Inkplate *_inkplatePtr, WiFiClass *_wifiPtr, uint8_t *_tempFbAddress)
{
    // Save these addresses locally.
    _framebufferHandler.framebuffer = _tempFbAddress;
    _wifi = _wifiPtr;
    _inkplate = _inkplatePtr;

    // Set the framebuffer size.
    _framebufferHandler.fbHeight = SCREEN_HEIGHT;
    _framebufferHandler.fbWidth = SCREEN_WIDTH;
}

bool ImageDecoder::draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format, enum InkplateImagePathType _PathType)
{
    // Check if the path detection is set to auto (it should be by default).
    if (_PathType == INKPLATE_IMAGE_DECODE_PATH_AUTO)
    {
        // Check if the path is web or microSD (local).
        bool _isWeb = Helpers::isWebPath((char*)_path);

        // If the path is truly web path, change it.
        _PathType = _isWeb ? INKPLATE_IMAGE_DECODE_PATH_WEB : INKPLATE_IMAGE_DECODE_PATH_SD;
    }

    // Check if the file format if set to auto. If not, it's manually overriden.
    if (_PathType == INKPLATE_IMAGE_DECODE_PATH_SD)
    {
        // Try to read at least 10 bytes from the file to check file format.
        // But first try to open the file.
        File _file = _inkplate->sdFat.open(_path);

        // If open failed, return error.
        if (_file)
        {
            if (_Format == INKPLATE_IMAGE_DECODE_FORMAT_AUTO)
            {
                uint8_t _fileSignature[10];

                // Sample 10 bytes from the file for the image format autodetect.
                _file.read(_fileSignature, 10);

                // Rewind it to the start.
                _file.rewind();
            
                // Auto-detect image format.
                _Format = Helpers::detectImageFormat((char*)_path, _fileSignature);
            }

            // Use proper decoder for the image type.
            bool _retValue = drawFromSd(&_file, _x, _y, _invert, _dither, _Format);

            // Close the file.
            _file.close();

            // Return from the method with decode status.
            return _retValue;
        }
        else
        {
            // File open failed!
            _DecodeError = INKPLATE_IMAGE_DECODE_ERR_FILE_OPEN_FAIL;
            return false;
        }
    }

    // If you got there, there must be something wrong.
    _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_HARD_FAULT;
    return false;
}

bool ImageDecoder::drawFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // To-Do.
}

bool ImageDecoder::drawFromSd(File *_file, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format)
{
    // Reset error variable.
    _DecodeError = INKPLATE_IMAGE_DECODE_NO_ERR;

    // Image width and height parameters (known after image decode).
    int _imageW = 0;
    int _imageH = 0;

    // Check the input parameters.
    if ((_file == NULL) || (_Format == INKPLATE_IMAGE_DECODE_FORMAT_ERR))
    {
        _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;
        return false;
    }

    // Get the file size.
    uint32_t _fileSize = _file->size();

    // Check what decoder needs to be used.
    switch (_Format)
    {
        case INKPLATE_IMAGE_DECODE_FORMAT_BMP:
        {
            // Let's initialize BMP decoder.
            memset(&_bmpDec, 0, sizeof(BmpDecode_t));
            BmpDecoderSessionHandler sessionData;
            _bmpDec.inputFeed = &readBytesFromSdBmp;
            _bmpDec.errorCode = BMP_DECODE_NO_ERROR;
            sessionData.file = _file;
            sessionData.frameBufferHandler = &_framebufferHandler;
            _bmpDec.output = &writeBytesToFrameBufferBmp;
            _bmpDec.sessionHandler = &sessionData;

            // Try to read if the file is vaild.
            if (!bmpDecodeVaildFile(&_bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

                // Return!
                return false;
            }

            // Try to decode a BMP header.
            if (!bmpDecodeProcessHeader(&_bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

                // Return!
                return false;
            }

            // Check if the BMP is a valid BMP for the decoder.
            if (!bmpDecodeVaildBMP(&_bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

                // Return!
                return false;
            }

            if (!bmpDecodeProcessBmp(&_bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

                // Return!
                return false;
            }

            // Check for bounds one more time.
            _imageW = min(1024, (int)(_bmpDec.header.infoHeader.width));
            _imageH = min(758, (int)(_bmpDec.header.infoHeader.height));
            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_JPG:
        {
            // Allocate the memory for the JPG decoder.
            const size_t _workingBufferSize = 4096;
            void *_workingBuffer;

            // Allocate the memory for the buffer.
            _workingBuffer = (void*)malloc(_workingBufferSize);
            if (_workingBuffer == NULL)
            {
                // Set the error.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_NO_MEMORY;

                // Return fail.
                return false;
            }

            // Initialize the JPG decoder.
            memset(&_jpgDecoder, 0, sizeof(JDEC));
            JdecIODev _sessionId;
            _sessionId.fp = _file;
            _sessionId.frameBufferHandler = &_framebufferHandler;
            JRESULT _result = jd_prepare(&_jpgDecoder, readBytesFromSdJpg, _workingBuffer, _workingBufferSize, &_sessionId);

            // Check if JPG decoder prepare is ok. If not, return error.
            if (_result == JDR_OK)
            {
                // Set output callback and decode the image!
                _result = jd_decomp(&_jpgDecoder, writeBytesToFrameBufferJpg, 0);

                // If failed, free memory and return fail.
                if (_result != JDR_OK)
                {
                    // Free allocated memory.
                    free(_workingBuffer);
                
                    // Set the error.
                    _DecodeError = INKPLATE_IMAGE_DECODE_ERR_JPG_DECODER_FAULT;

                    // Return fail.
                    return false;
                }
            }
            else
            {
                // Free allocated memory.
                free(_workingBuffer);

                // Set the error.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_NO_MEMORY;

                // Return fail.
                return false;
            }

            _imageW = min(1024, (int)(_jpgDecoder.width));
            _imageH = min(758, (int)(_jpgDecoder.height));

            // Free up the memory.
            free(_workingBuffer);
            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_PNG:
        {
            // Decode it chunk-by-chunk.
            // Allocate new PNG decoder.
            _newPngle = pngle_new();

            // Add session handler for the framebuffer access.
            BmpDecoderSessionHandler sessionHandler;
            sessionHandler.frameBufferHandler = &_framebufferHandler;
            pngle_set_session_handle(_newPngle, &sessionHandler);

            // Set the callback for decoder.
            pngle_set_draw_callback(_newPngle, myPngleOnDraw);

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
                    int fed = pngle_feed(_newPngle, buff, len);
                    if (fed < 0)
                    {
                        // Free up the memory.
                        pngle_destroy(_newPngle);

                        // Set error.
                        _DecodeError = INKPLATE_IMAGE_DECODE_ERR_PNG_DECODER_FAULT;

                        // Go back!
                        return false;
                    }
                    remain = remain + len - fed;
                    pnt += len;
                }
            }

            // Get the width and height of the decoded image.
            _imageW = pngle_get_width(_newPngle);
            _imageH = pngle_get_height(_newPngle);

            // Free up the memory after succ decode.
            pngle_destroy(_newPngle);
            break;
        }
    }

    // For testing only!
    RGBtoGrayscale(_inkplate, _x, _y, _framebufferHandler.framebuffer, _imageW, _imageH);

    // Everything went ok? Return success!
    return true;
}

bool ImageDecoder::drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // To-Do.
}

enum InkplateImageDecodeErrors ImageDecoder::getError()
{
    // Return last error.
    return _DecodeError;
}


// To-Do: MOVE THIS IN THE SEPEARTE FILE!!!! FOR TESTING ONLY!
void ImageDecoder::RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h)
{
    for (int _y = 0; _y < _h; _y++)
    {
        for (int _x = 0; _x < _w; _x++)
        {
            // Calculate the framebuffer array index.
            uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

            // get the individual RGB colors.
            uint8_t _r = _rgbBuffer[_fbArrayIndex + 2];
            uint8_t _g = _rgbBuffer[_fbArrayIndex + 1];
            uint8_t _b = _rgbBuffer[_fbArrayIndex];

            // Convert them into grayscale.
            uint8_t _pixel = ((77 * _r) + (150 * _g) + (29 * _b)) >> 12;

            // Write into epaper framebuffer.
            _inkplate->drawPixel(_x + x, _y + y, _pixel);
        }
    }
}

#endif