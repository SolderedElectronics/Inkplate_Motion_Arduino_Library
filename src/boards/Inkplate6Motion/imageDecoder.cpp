// Include library header file.
#include "imageDecoder.h"

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include all needed image decoders.
#include "../../libs/bmpDecode/bmpDecode.h"
#include "../../libs/TJpgDec/tjpgd.h"
#include "../../libs/pngle/pngle.h"

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

void ImageDecoder::begin(Inkplate *_inkplate, SdFat *_sdFatPtr, uint8_t *_tempFbAddress)
{
    // Save these addresses locally.
    _framebufferHandler.framebuffer = _tempFbAddress;
    _sdFat = _sdFatPtr;
    _inkplatePtr = _inkplate;

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
    // To-Do

    // Use proper decoder for the image type.
    return drawFromSd(_path, _x, _y, _invert, _dither, _Format);
}

bool ImageDecoder::drawFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _format)
{
    // To-Do.
}

bool ImageDecoder::drawFromSd(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format)
{
    // Reset error variable.
    _DecodeError = INKPLATE_IMAGE_DECODE_NO_ERR;

    // Image width and height parameters (known after image decode).
    int _imageW = 0;
    int _imageH = 0;

    // Check the input parameters.
    if ((_path == NULL) || (_Format == INKPLATE_IMAGE_DECODE_FORMAT_ERR))
    {
        _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;
        return false;
    }

    // First, open the file.
    File _file = _sdFat->open(_path, O_READ);

    // Check for the file open success.
    if (!_file)
    {
        // File open failed? Return false and set the error.
        _DecodeError = INKPLATE_IMAGE_DECODE_ERR_FILE_OPEN_FAIL;
        return false;
    }

    // Get the file size.
    uint32_t _fileSize = _file.size();

    // Check what decoder needs to be used.
    switch (_Format)
    {
        case INKPLATE_IMAGE_DECODE_FORMAT_BMP:
        {
            // Let's initialize BMP decoder.
            BmpDecode_t bmpDec;
            BmpDecoderSessionHandler sessionData;
            bmpDec.inputFeed = &readBytesFromSdBmp;
            bmpDec.errorCode = BMP_DECODE_NO_ERROR;
            sessionData.file = &_file;
            sessionData.frameBufferHandler = &_framebufferHandler;
            bmpDec.output = &writeBytesToFrameBufferBmp;
            bmpDec.sessionHandler = &sessionData;

            // Try to read if the file is vaild.
            if (!bmpDecodeVaildFile(&bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_ERROR;
                
                // Close the file.
                _file.close();

                // Return!
                return false;
            }

            // Try to decode a BMP header.
            if (!bmpDecodeProcessHeader(&bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_ERROR;
                
                // Close the file.
                _file.close();

                // Return!
                return false;
            }

            // Check if the BMP is a valid BMP for the decoder.
            if (!bmpDecodeVaildBMP(&bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_ERROR;
                
                // Close the file.
                _file.close();

                // Return!
                return false;
            }

            if (!bmpDecodeProcessBmp(&bmpDec))
            {
                // Set error while drawing image.
                _DecodeError = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_ERROR;
                
                // Close the file.
                _file.close();

                // Return!
                return false;
            }

            // Check for bounds one more time.
            _imageW = min(1024, (int)(bmpDec.header.infoHeader.width));
            _imageH = min(758, (int)(bmpDec.header.infoHeader.height));
            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_JPG:
        {

            break;
        }
        case INKPLATE_IMAGE_DECODE_FORMAT_PNG:
        {

            break;
        }
    }

    // For testing only!
    RGBtoGrayscale(_inkplatePtr, _x, _y, _framebufferHandler.framebuffer, _imageW, _imageH);

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