/**
 **************************************************
 *
 * @file        imageDecoder.cpp
 * @brief       Board specific image decoder source file for
 *              ImageDecoder class. Used by the main class
 *              for image decoding from the microSD card
 *              or from the web.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include library header file.
#include "imageDecoder.h"

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include code for all image decoder callbacks.
#include "imageDecoderCallbacks.h"

// It needs also main Inkplate Motion file to get the base class.
#include "../../InkplateMotion.h"

/**
 * @brief Construct a new Image Decoder::Image Decoder object
 *
 */
ImageDecoder::ImageDecoder()
{
    // Empty for now.
}

/**
 * @brief   Initializer for the Inkplate Image decoder class.
 *
 * @param   Inkplate *_inkplatePtr
 *          Pointer for the main Inkplate object (used for drawPixel() method).
 * @param   WiFiClass *_wifiPtr
 *          Pointer for the WiFi Class object.
 * @param   uint8_t *_tempFbAddress
 *          SDRAM address for temp. storing decoded image.
 * @param   volatile uint8_t *_downloadFileMemory
 *          The address to where image files from web should be downloaded
 *
 */
void ImageDecoder::begin(Inkplate *_inkplatePtr, WiFiClass *_wifiPtr, ImageProcessing *_imgProcessPtr,
                         uint8_t *_tempFbAddress, volatile uint8_t *_downloadFileMemory)
{
    // Save these addresses locally.
    _framebufferHandler.framebuffer = _tempFbAddress;
    _wifi = _wifiPtr;
    _inkplate = _inkplatePtr;
    _imgProcess = _imgProcessPtr;
    _imageDownloadMemoryPtr = _downloadFileMemory;

    // Set the framebuffer size.
    _framebufferHandler.fbHeight = SCREEN_HEIGHT;
    _framebufferHandler.fbWidth = SCREEN_WIDTH;
}

/**
 * @brief   Loads the image from the microSD card or from the web, decodes it and stores decoded image
 *          in the main epaper framebuffer.
 *
 * @param   const char *_path
 *          Path and filename to the image. Can be link from the web or path to the image on the microSD card.
 * @param   int _x
 *          X position of the image in the ePaper framebuffer.
 * @param   int _y
 *          Y position of the image in the ePaper framebuffer.
 * @param   bool _invert
 *          true - colors are inverted.
 * @param   uint8_t _dither
 *          Disable or enable dithering on the image as well as choosing dither kernel.
 * @param   enum InkplateImageDecodeFormat _format
 *          Force specific image format (if automatic detecton of the image format fails).
 * @param   enum InkplateImagePathType _pathType
 *          Force specific path where image is stored (web or microSD).
 * @return  bool
 *          true - Image loaded in the ePaper framebuffer succ.
 *          false - Image load failed. Check ImageDecoder::getError() for the reason.
 */
bool ImageDecoder::draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither,
                        const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize,
                        enum InkplateImageDecodeFormat _format, enum InkplateImagePathType _pathType)
{
    // Check if the path detection is set to auto (it should be by default).
    if (_pathType == INKPLATE_IMAGE_DECODE_PATH_AUTO)
    {
        // Check if the path is web or microSD (local).
        bool _isWeb = inkplateImageDecodeHelpersIsWebPath((char *)_path);

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
                _format = inkplateImageDecodeHelpersDetectImageFormat((char *)_path, _fileSignature);
            }

            // Use proper decoder for the image type.
            bool _retValue = drawFromSd(&_file, _x, _y, _invert, _dither, _ditherKernelParameters,
                                        _ditherKernelParametersSize, _format);

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
        // Call drawFromWeb and return the result of that
        return drawFromWeb(_path, _x, _y, _invert, _dither, _ditherKernelParameters, _ditherKernelParametersSize,
                           _format);
    }

    // If you got there, there must be something wrong.
    _decodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_HARD_FAULT;
    return false;
}

/**
 * @brief   Loads image stored in SDRAM/SRAM/FLASH as image format (not RAW bitmap data ready for the
 *          framebuffer).
 *
 * @param   void *buffer
 *
 * @param   int _x
 *          X position of the image in the ePaper framebuffer.
 * @param   int _y
 *          Y position of the image in the ePaper framebuffer.
 * @param   bool _invert
 *          true - colors are inverted.
 * @param   uint8_t _dither
 *          Disable or enable dithering on the image as well as choosing dither kernel.
 * @param   enum InkplateImageDecodeFormat _format
 *          Force specific image format (if automatic detecton of the image format fails).
 * @return  bool
 *          true - Image loaded in the ePaper framebuffer succ.
 *          false - Image load failed. Check ImageDecoder::getError() for the reason.
 */
bool ImageDecoder::drawFromBuffer(void *_buffer, size_t _size, int _x, int _y, bool _invert, uint8_t _dither,
                                  const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize,
                                  enum InkplateImageDecodeFormat _format)
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
    case INKPLATE_IMAGE_DECODE_FORMAT_BMP: {
        // Let's initialize BMP decoder.
        memset(&_bmpDecoder, 0, sizeof(BmpDecode_t));
        InkplateDecoderSessionHandler _sessionHandler;
        _bmpDecoder.inputFeed = &readBytesFromBufferBmp;
        _bmpDecoder.errorCode = BMP_DECODE_NO_ERROR;
        _sessionHandler.fileBuffer = (uint8_t *)_buffer;
        _sessionHandler.frameBufferHandler = &_framebufferHandler;
        _sessionHandler.bufferOffset = 0;
        _sessionHandler.fileBufferSize = _size;
        _bmpDecoder.output = &writeBytesToFrameBufferBmp;
        _bmpDecoder.sessionHandler = &_sessionHandler;

        // Call function to process BMP decoding.
        if (!inkplateImageDecodeHelpersBmp(&_bmpDecoder, &_decodeError))
        {
            // Something went wrong!
            return false;
        }

        // Check image size and constrain it.
        _imageW = min((int)(SCREEN_WIDTH), (int)(_bmpDecoder.header.infoHeader.width));
        _imageH = min((int)(SCREEN_HEIGHT), (int)(_bmpDecoder.header.infoHeader.height));
        break;
    }

    case INKPLATE_IMAGE_DECODE_FORMAT_JPG: {
        // Initialize the JPG decoder.
        memset(&_jpgDecoder, 0, sizeof(JDEC));
        InkplateDecoderSessionHandler _sessionHandler;
        _sessionHandler.fileBuffer = (uint8_t *)_buffer;
        _sessionHandler.frameBufferHandler = &_framebufferHandler;
        _sessionHandler.bufferOffset = 0;
        _sessionHandler.fileBufferSize = _size;

        if (!inkplateImageDecodeHelpersJpg(&_jpgDecoder, &readBytesFromBufferJpg, &writeBytesToFrameBufferJpg,
                                           &_decodeError, &_sessionHandler))
        {
            // Something went wrong with decoding the image!
            return false;
        }

        // Check image size and constrain it.
        _imageW = min((int)(SCREEN_WIDTH), (int)(_jpgDecoder.width));
        _imageH = min((int)(SCREEN_HEIGHT), (int)(_jpgDecoder.height));
        break;
    }

    case INKPLATE_IMAGE_DECODE_FORMAT_PNG: {
        // Create session handler.
        InkplateDecoderSessionHandler _sessionHandler;
        _sessionHandler.fileBuffer = (uint8_t *)_buffer;
        _sessionHandler.frameBufferHandler = &_framebufferHandler;
        _sessionHandler.bufferOffset = 0;
        _sessionHandler.fileBufferSize = _size;

        // Decode it chunk-by-chunk.
        if (!inkplateImageDecodeHelpersPng(_pngDecoder, &readBytesFromBufferPng, &writeBytesToFrameBufferPng, &_imageW,
                                           &_imageH, &_decodeError, &_sessionHandler))
        {
            // Something went wrong with decoding the image
            return false;
        }

        // Check image size and constrain it.
        _imageW = min((int)(SCREEN_WIDTH), (int)(_imageW));
        _imageH = min((int)(SCREEN_HEIGHT), (int)(_imageH));
        break;
    }

    default: {
        // Somehow no format specified, return error!
        _decodeError = INKPLATE_IMAGE_DECODE_ERR_UNKNOWN_FORMAT;
        return false;
    }
    }

    // Process the image and draw it in the epaper framebuffer.
    _imgProcess->processImage((uint8_t *)(_framebufferHandler.framebuffer), _x, _y, _imageW, _imageH, _dither, _invert,
                              _ditherKernelParameters, _ditherKernelParametersSize,
                              _inkplate->getDisplayMode() == INKPLATE_1BW ? 1 : 4);

    // Decoded ok? Return true!
    return true;
}

/**
 * @brief   Loads and decodes image from the microSD. This is usually called from ImageDecoder::draw().
 *
 * @param   File * _file
 *          Pointer to the SdFat file object. File must be opened!
 * @param   int _x
 *          X position of the image in the epaper framebuffer.
 * @param   int _y
 *          Y position of the image in the epaper framebuffer.
 * @param   bool _invert
 *          true - Colors are inverted.
 * @param   uint8_t _dither
 *          Disable or enable dithering on the image as well as choosing dither kernel.
 * @param   enum InkplateImageDecodeFormat _format
 *          Force specific image format (if automatic detecton of the image format fails).
 * @return  bool
 *          true - Image loaded and decoded succ.
 *          false -  Image load/decode failed. Check ImageDecoder::getError() for reason.
 */
bool ImageDecoder::drawFromSd(File *_file, int _x, int _y, bool _invert, uint8_t _dither,
                              const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize,
                              enum InkplateImageDecodeFormat _format)
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
    case INKPLATE_IMAGE_DECODE_FORMAT_BMP: {
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
    case INKPLATE_IMAGE_DECODE_FORMAT_JPG: {
        // Initialize the JPG decoder.
        memset(&_jpgDecoder, 0, sizeof(JDEC));
        InkplateDecoderSessionHandler _sessionHandler;
        _sessionHandler.file = _file;
        _sessionHandler.frameBufferHandler = &_framebufferHandler;

        if (!inkplateImageDecodeHelpersJpg(&_jpgDecoder, &readBytesFromSdJpg, &writeBytesToFrameBufferJpg,
                                           &_decodeError, &_sessionHandler))
            return false;

        // Check image size and constrain it.
        _imageW = min((int)(SCREEN_WIDTH), (int)(_jpgDecoder.width));
        _imageH = min((int)(SCREEN_HEIGHT), (int)(_jpgDecoder.height));
        break;
    }
    case INKPLATE_IMAGE_DECODE_FORMAT_PNG: {
        // Create session handler.
        InkplateDecoderSessionHandler _sessionHandler;
        _sessionHandler.file = _file;
        _sessionHandler.frameBufferHandler = &_framebufferHandler;

        // Decode it chunk-by-chunk.
        if (!inkplateImageDecodeHelpersPng(_pngDecoder, readBytesFromSdPng, writeBytesToFrameBufferPng, &_imageW,
                                           &_imageH, &_decodeError, &_sessionHandler))
            return false;

        // Check image size and constrain it.
        _imageW = min((int)(SCREEN_WIDTH), (int)(_imageW));
        _imageH = min((int)(SCREEN_HEIGHT), (int)(_imageH));
        break;
    }

    default: {
        // Set error flag.
        _decodeError = INKPLATE_IMAGE_DECODE_ERR_UNKNOWN_FORMAT;

        // Return false for fail.
        return false;
    }
    }

    // Process the image and draw it in the epaper framebuffer.
    _imgProcess->processImage((uint8_t *)(_framebufferHandler.framebuffer), _x, _y, _imageW, _imageH, _dither, _invert,
                              _ditherKernelParameters, _ditherKernelParametersSize,
                              _inkplate->getDisplayMode() == INKPLATE_1BW ? 1 : 4);

    // Everything went ok? Return success!
    return true;
}

/**
 * @brief This function draws an image from web, it has format auto-detection. Max image file size is by default 4MB.
 *
 * @note Not all image formats and HTTP servers are created equal. There is support for all these image formats, but the
 * software can't handle every possible case. If your image doesn't work, please try exporting it from a different image
 * editing program and try a different host.
 *
 * @param   const char * _path
 *          The URL to the image to be drawn
 * @param   int _x
 *          The x coordinate of where to draw the image on the display
 * @param   int _y
 *          The y coordinate of where to draw the image on the display
 * @param   bool _invert
 *          To invert the image or not (true inverts, false does not)
 * @param   bool _dither
 *          To dither hte image or not (true dithers, false does not)
 * @param   enum InkplateImageDecodeFormat _format
 *          The format of the image, you can pass AUTO and it will be detected in-function. Check the enum for more
 * details
 *
 * @return  bool
 *          True if everything was successful, false if something went wront
 *
 */
bool ImageDecoder::drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither,
                               const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize,
                               enum InkplateImageDecodeFormat _format)
{
    // Let's download  the file and save it to the image download memory
    WiFiClient client;
    uint32_t fileSize = client.downloadFile(_path, _imageDownloadMemoryPtr, DOWNLOAD_IMAGE_MAX_SIZE); // 4MB buffer size

    // Didn't download file? There was an error!
    if (fileSize == 0)
        return false;

    // Let's check if format needs to be detected
    if (_format == INKPLATE_IMAGE_DECODE_FORMAT_AUTO)
    {
        // Format needs to be detected
        // First, copy the first 10 bytes to a buffer which will be used to read the file signature
        uint8_t fileHeader[10];
        memcpy(fileHeader, (void *)_imageDownloadMemoryPtr, 10);
        _format = inkplateImageDecodeHelpersDetectImageFormat((char *)_path, fileHeader);
    }

    // Now, draw the image from the buffer and return the result of that
    return drawFromBuffer((void *)_imageDownloadMemoryPtr, fileSize, _x, _y, _invert, _dither, _ditherKernelParameters,
                          _ditherKernelParametersSize, _format);
}

/**
 * @brief   Returns error while decoding image (with ImageDecoder::draw()).
 *          If no error, it will return INKPLATE_IMAGE_DECODE_NO_ERR.
 *          All errors can be found in system/inkplateImageDecoderHelpers.h.
 *          All errors are cleared in ImageDecoder::draw() before decode and load
 *          process.
 *
 * @return  enum InkplateImageDecodeErrors
 */
enum InkplateImageDecodeErrors ImageDecoder::getError()
{
    // Return last error.
    return _decodeError;
}

#endif