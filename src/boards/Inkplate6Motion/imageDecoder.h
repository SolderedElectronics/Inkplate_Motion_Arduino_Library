// Add header guard.
#ifndef __INKPLATE_MOTION_IMAGE_DECODE_H__
#define __INKPLATE_MOTION_IMAGE_DECODE_H__

// Include heplers.
#include "../../system/helpers.h"

// Include file for the each board feature selection.
#include "features/featureSelect.h"

class Inkplate;

class ImageDecoder
{
  public:
    ImageDecoder();
    void begin(Inkplate *_inkplate, SdFat *_sdFatPtr, uint8_t *_tempFbAddress);
    bool draw(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO, enum inkplateImagePathType _pathType = INKPLATE_IMAGE_DECODE_PATH_AUTO);
    bool drawFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO);
    bool drawFromSd(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format);
    bool drawFromWeb(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format);

    void RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h);
    enum inkplateImageDecodeErrors getError();

  private:
    SdFat *_sdFat;
    Inkplate *_inkplatePtr;
    volatile uint8_t *_decodedImageFb;
    enum inkplateImageDecodeErrors _decodeError = INKPLATE_IMAGE_DECODE_ERR_OK;
};

#endif