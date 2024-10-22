// Add header guard.
#ifndef __INKPLATE_MOTION_IMAGE_DECODE_H__
#define __INKPLATE_MOTION_IMAGE_DECODE_H__

// Include Arduino Header file.
#include <Arduino.h>

// Include helpers.


class ImageDecoder
{
  public:
    ImageDecoder(uint32_t _tempFbAddress);
    bool drawImage(const char *_path, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO, enum inkplateImagePathType _pathType = INKPLATE_IMAGE_DECODE_PATH_AUTO);
    bool drawImageFromBuffer(void *_buffer, int _x, int _y, bool _invert, uint8_t _dither, enum inkplateImageDecodeFormat _format = INKPLATE_IMAGE_DECODE_FORMAT_AUTO);
    enum inkplateImageDecodeErrors getError();

  private:
    volatile uint32_t _decodedImageFb;
};

#endif