/**
 **************************************************
 *
 * @file        imageDecoder.h
 * @brief       Board specific image decoder header file for 
 *              ImageDecoder class. Used by the main class
 *              for image decoding from the microSD card
 *              or from the web.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Add header guard.
#ifndef __INKPLATE_MOTION_IMAGE_DECODE_H__
#define __INKPLATE_MOTION_IMAGE_DECODE_H__

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include heplers.
#include "../../system/inkplateImageDecoderHelpers.h"

// Include file for the each board feature selection.
#include "features/featureSelect.h"

// Include Image Processing library as well.
#include "../../libs/imageProcessing/imageProcessing.h"

// Define the maximum downloadable file size for images (for drawImageFromWeb and draw functions)
#define DOWNLOAD_IMAGE_MAX_SIZE 4*1024*1024 // 4MB by default

// Forward declaration of the Inkplate Class and WiFiClass.
class Inkplate;
class WiFiClass;

// Image decode framebuffer typedef.
typedef struct
{
    uint16_t fbWidth;
    uint16_t fbHeight; 
    volatile uint8_t *framebuffer;
}InkplateImageDecodeFBHandler;

// Image decoder class - handles everything for displaying image on the screen from microSD card, WiFi or buffer.
class ImageDecoder
{
  public:
    ImageDecoder();
    void begin(Inkplate *_inkplatePtr, WiFiClass *_wifiPtr, ImageProcessing *_imgProcessPtr, uint8_t *_tempFbAddress, volatile uint8_t * _downloadFileMemory);
    bool draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, enum InkplateImageDecodeFormat _format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO, enum InkplateImagePathType _pathType = INKPLATE_IMAGE_DECODE_PATH_AUTO);
    bool drawFromBuffer(void *_buffer, size_t _size, int _x, int _y, bool _invert, uint8_t _dither, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, enum InkplateImageDecodeFormat _format);
    bool drawFromSd(File *_file, int _x, int _y, bool _invert, uint8_t _dither, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, enum InkplateImageDecodeFormat _format);
    bool drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, enum InkplateImageDecodeFormat _format);

    // Get what kind of error ImageDecode class got.
    // Note that there is seperate method for the each image decoder errors.
    enum InkplateImageDecodeErrors getError();

  private:
    // Inkplate base class object pointer - needed for Inkplate::drawPixel();
    Inkplate *_inkplate;

    // WiFiClass pointer to the object.
    WiFiClass *_wifi;

    // Class for the image processing and displying content on the screen after decode.
    ImageProcessing *_imgProcess;

    // Framebuffer handler.
    InkplateImageDecodeFBHandler _framebufferHandler;

    // Handle for each of the decoders.
    BmpDecode_t _bmpDecoder;
    JDEC _jpgDecoder;
    pngle_t *_pngDecoder;

    // Pointer to memory where images are downloaded via Wifi
    volatile uint8_t * _imageDownloadMemoryPtr;

    // Decoded image error log.
    enum InkplateImageDecodeErrors _decodeError = INKPLATE_IMAGE_DECODE_NO_ERR;
};

#endif

#endif