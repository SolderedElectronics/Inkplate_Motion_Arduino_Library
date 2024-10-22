// Include library header file.
#include "imageDecoder.h"

// Include heplers in source file.
#include "../../system/helpers.h"

/**
 * @brief Construct a new Image Decoder:: Image Decoder object
 * 
 * @param _tempFbAddress 
 */
ImageDecoder::ImageDecoder(uint32_t _tempFbAddress)
{
    // Save this address locally.
    _decodedImageFb = _tempFbAddress;
}

bool ImageDecoder::drawImage(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format, enum inkplateImagePathType _pathType)
{
    // Check if the path detection is set to auto (it should be by default).
    if (_pathType == INKPLATE_IMAGE_DECODE_PATH_AUTO)
    {
        // Check if the path is web or microSD (local).
        bool _isWeb = Helpers::isWebPath((char*)_path);

        // If the path is truly web path, change it.
        _pathType = _isWeb ? INKPLATE_IMAGE_DECODE_PATH_WEB : INKPLATE_IMAGW_DECODE_PATH_SD;
    }

    // Check if the file format if set to auto. If not, it's manually overriden.
    if (_format == INKPLATE_IMAGE_DECODE_FORMAT_AUTO)
    {
        
    }
}

bool ImageDecoder::drawImageFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format)
{

}

enum inkplateImageDecodeErrors ImageDecoder::getError()
{

}