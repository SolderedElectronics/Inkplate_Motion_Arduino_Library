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

/**
 * @brief   
 * 
 * @param   _inkplatePtr 
 * @param   _wifiPtr 
 * @param   _tempFbAddress 
 */
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

/**
 * @brief 
 * 
 * @param _path 
 * @param _x 
 * @param _y 
 * @param _invert 
 * @param _dither 
 * @param _format 
 * @param _pathType 
 * @return true 
 * @return false 
 */
bool ImageDecoder::draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format, enum InkplateImagePathType _pathType)
{
    // Check if the path detection is set to auto (it should be by default).
    if (_pathType == INKPLATE_IMAGE_DECODE_PATH_AUTO)
    {
        // Check if the path is web or microSD (local).
        bool _isWeb = inkplateImageDecodeHelpersIsWebPath((char*)_path);

        // If the path is truly web path, change it.
        _pathType = _isWeb ? INKPLATE_IMAGE_DECODE_PATH_WEB : INKPLATE_IMAGE_DECODE_PATH_SD;
    }

    // Check if the file format if set to auto. If not, it's manually overriden.
    if (_pathType == INKPLATE_IMAGE_DECODE_PATH_SD)
    {
        // Try to read at least 10 bytes from the file to check file format.
        // But first try to open the file.
        File _file = _inkplate->sdFat.open(_path);

        // If open failed, return error.
        if (_file)
        {
            if (_format == INKPLATE_IMAGE_DECODE_FORMAT_AUTO)
            {
                uint8_t _fileSignature[10];

                // Sample 10 bytes from the file for the image format autodetect.
                _file.read(_fileSignature, 10);

                // Rewind it to the start.
                _file.rewind();
            
                // Auto-detect image format.
                _format = inkplateImageDecodeHelpersDetectImageFormat((char*)_path, _fileSignature);
            }

            // Use proper decoder for the image type.
            bool _retValue = drawFromSd(&_file, _x, _y, _invert, _dither, _format);

            // Close the file.
            _file.close();

            // Return from the method with decode status.
            return _retValue;
        }
        else
        {
            // File open failed!
            _decodeError = INKPLATE_IMAGE_DECODE_ERR_FILE_OPEN_FAIL;
            return false;
        }
    }
    else if (_pathType == INKPLATE_IMAGE_DECODE_PATH_WEB)
    {
        // This is web path. To-Do.
    }

    // If you got there, there must be something wrong.
    _decodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_HARD_FAULT;
    return false;
}

/**
 * @brief 
 * 
 * @param _buffer 
 * @param _x 
 * @param _y 
 * @param _invert 
 * @param _dither 
 * @param _format 
 * @return true 
 * @return false 
 */
bool ImageDecoder::drawFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // Watch-out! Some decoders have some issues while reading directly from the SDRAM. I'm not sure why...
    // Clear all errors.
    _decodeError = INKPLATE_IMAGE_DECODE_NO_ERR;

    // Image width and height parameters (known after image decode).
    int _imageW = 0;
    int _imageH = 0;
    
    // Check for the null pointer.
    if (_buffer == NULL)
    {
        // Set error flag.
        _decodeError = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;

        // Go back!
        return false;
    }

    // Set the decoder.
    // Code is similar as for the microSD card, but uses different callbacks and sessionHandlers.
    switch (_format)
    {
        case INKPLATE_IMAGE_DECODE_FORMAT_BMP:
        {
            // Let's initialize BMP decoder.
            memset(&_bmpDecoder, 0, sizeof(BmpDecode_t));
            InkplateDecoderSessionHandler _sessionHandler;
            _bmpDecoder.inputFeed = &readBytesFromBufferBmp;
            _bmpDecoder.errorCode = BMP_DECODE_NO_ERROR;
            _sessionHandler.fileBuffer = (uint8_t*)_buffer;
            _sessionHandler.frameBufferHandler = &_framebufferHandler;
            _bmpDecoder.output = &writeBytesToFrameBufferBmp;
            _bmpDecoder.sessionHandler = &_sessionHandler;

            // Call function to process BMP decoding.
            if (!inkplateImageDecodeHelpersBmp(&_bmpDecoder, &_decodeError))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_bmpDecoder.header.infoHeader.width));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_bmpDecoder.header.infoHeader.height));
            break;
        }

        case INKPLATE_IMAGE_DECODE_FORMAT_JPG:
        {
            // Initialize the JPG decoder.
            memset(&_jpgDecoder, 0, sizeof(JDEC));
            InkplateDecoderSessionHandler _sessionHandler;
            _sessionHandler.fileBuffer = (uint8_t*)(_buffer);
            _sessionHandler.frameBufferHandler = &_framebufferHandler;

            if (!inkplateImageDecodeHelpersJpg(&_jpgDecoder, &readBytesFromBufferJpg, &writeBytesToFrameBufferJpg, &_decodeError, &_sessionHandler))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_jpgDecoder.width));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_jpgDecoder.height));
            break;
        }

        case INKPLATE_IMAGE_DECODE_FORMAT_PNG:
        {
            // Create session handler.
            InkplateDecoderSessionHandler _sessionHandler;
            _sessionHandler.fileBuffer = (uint8_t*)(_buffer);
            _sessionHandler.frameBufferHandler = &_framebufferHandler;

            // Decode it chunk-by-chunk.
            if (!inkplateImageDecodeHelpersPng(_pngDecoder, &readBytesFromBufferPng, &writeBytesToFrameBufferPng, &_imageW, &_imageH, &_decodeError, &_sessionHandler))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_imageW));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_imageH));
            break;
        }

        default:
        {
            // Somehow no format specified, return error!
            _decodeError = INKPLATE_IMAGE_DECODE_ERR_UNKNOWN_FORMAT;
            return false;
        }
    }

    // Decoded ok? Return true!
    return true;
}

/**
 * @brief   
 * 
 * @param _file 
 * @param _x 
 * @param _y 
 * @param _invert 
 * @param _dither 
 * @param _format 
 * @return true 
 * @return false 
 */
bool ImageDecoder::drawFromSd(File *_file, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // Reset error variable.
    _decodeError = INKPLATE_IMAGE_DECODE_NO_ERR;

    // Image width and height parameters (known after image decode).
    int _imageW = 0;
    int _imageH = 0;

    // Check the input parameters.
    if ((_file == NULL) || (_format == INKPLATE_IMAGE_DECODE_FORMAT_ERR))
    {
        _decodeError = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;
        return false;
    }

    // Get the file size.
    uint32_t _fileSize = _file->size();

    // Check what decoder needs to be used.
    switch (_format)
    {
        case INKPLATE_IMAGE_DECODE_FORMAT_BMP:
        {
            // Let's initialize BMP decoder.
            memset(&_bmpDecoder, 0, sizeof(BmpDecode_t));
            InkplateDecoderSessionHandler _sessionHandler;
            _bmpDecoder.inputFeed = &readBytesFromSdBmp;
            _bmpDecoder.errorCode = BMP_DECODE_NO_ERROR;
            _sessionHandler.file = _file;
            _sessionHandler.frameBufferHandler = &_framebufferHandler;
            _bmpDecoder.output = &writeBytesToFrameBufferBmp;
            _bmpDecoder.sessionHandler = &_sessionHandler;

            // Call function to process BMP decoding.
            if (!inkplateImageDecodeHelpersBmp(&_bmpDecoder, &_decodeError))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_bmpDecoder.header.infoHeader.width));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_bmpDecoder.header.infoHeader.height));
            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_JPG:
        {
            // Initialize the JPG decoder.
            memset(&_jpgDecoder, 0, sizeof(JDEC));
            InkplateDecoderSessionHandler _sessionHandler;
            _sessionHandler.file = _file;
            _sessionHandler.frameBufferHandler = &_framebufferHandler;

            if (!inkplateImageDecodeHelpersJpg(&_jpgDecoder, &readBytesFromSdJpg, &writeBytesToFrameBufferJpg, &_decodeError, &_sessionHandler))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_jpgDecoder.width));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_jpgDecoder.height));
            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_PNG:
        {
            // Create session handler.
            InkplateDecoderSessionHandler _sessionHandler;
            _sessionHandler.file = _file;
            _sessionHandler.frameBufferHandler = &_framebufferHandler;

            // Decode it chunk-by-chunk.
            if (!inkplateImageDecodeHelpersPng(_pngDecoder, readBytesFromSdPng, writeBytesToFrameBufferPng, &_imageW, &_imageH, &_decodeError, &_sessionHandler))
                return false;

            // Check image size and constrain it.
            _imageW = min((int)(SCREEN_WIDTH), (int)(_imageW));
            _imageH = min((int)(SCREEN_HEIGHT), (int)(_imageH));
            break;
        }

        default:
        {
            // Set error flag.
            _decodeError = INKPLATE_IMAGE_DECODE_ERR_UNKNOWN_FORMAT;

            // Return false for fail.
            return false;
        }
    }

    // For testing only!
    RGBtoGrayscale(_inkplate, _x, _y, _framebufferHandler.framebuffer, _imageW, _imageH);

    // Everything went ok? Return success!
    return true;
}

/**
 * @brief 
 * 
 * @param _path 
 * @param _x 
 * @param _y 
 * @param _invert 
 * @param _dither 
 * @param _format 
 * @return true 
 * @return false 
 */
bool ImageDecoder::drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // To-Do.
}

/**
 * @brief 
 * 
 * @return enum InkplateImageDecodeErrors 
 */
enum InkplateImageDecodeErrors ImageDecoder::getError()
{
    // Return last error.
    return _decodeError;
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