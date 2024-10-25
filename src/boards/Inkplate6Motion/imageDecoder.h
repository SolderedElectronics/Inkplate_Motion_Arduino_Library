// Add header guard.
#ifndef __INKPLATE_MOTION_IMAGE_DECODE_H__
#define __INKPLATE_MOTION_IMAGE_DECODE_H__

// Block usage on other boards.
#ifdef BOARD_INKPLATE6_MOTION

// Include heplers.
#include "../../system/helpers.h"

// Include file for the each board feature selection.
#include "features/featureSelect.h"

// Include all needed image decoders.
#include "../../libs/bmpDecode/bmpDecode.h"
#include "../../libs/TJpgDec/tjpgd.h"
#include "../../libs/pngle/pngle.h"

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
    void begin(Inkplate *_inkplatePtr, WiFiClass *_wifiPtr, uint8_t *_tempFbAddress);
    bool draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO, enum InkplateImagePathType _PathType = INKPLATE_IMAGE_DECODE_PATH_AUTO);
    bool drawFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO);
    bool drawFromSd(File *_file, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format);
    bool drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum InkplateImageDecodeFormat _Format);

    // MOVE THIS INTO IMAGE PROCESSING!
    void RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h);

    // Get what kind of error ImageDecode class got.
    // Note that there is seperate method for the each image decoder errors.
    enum InkplateImageDecodeErrors getError();

  private:
    // Inkplate base class object pointer - needed for Inkplate::drawPixel();
    Inkplate *_inkplate;

    // WiFiClass pointer to the object.
    WiFiClass *_wifi;

    // Framebuffer handler.
    InkplateImageDecodeFBHandler _framebufferHandler;

    // Handle for each of the decoders.
    BmpDecode_t _bmpDec;
    JDEC _jpgDecoder;
    pngle_t *_newPngle;

    // Decoded image error log.
    enum InkplateImageDecodeErrors _DecodeError = INKPLATE_IMAGE_DECODE_NO_ERR;
};

#endif

#endif