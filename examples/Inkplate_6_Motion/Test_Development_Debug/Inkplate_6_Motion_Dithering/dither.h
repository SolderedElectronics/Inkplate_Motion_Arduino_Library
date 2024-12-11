#ifndef __INKPLATE_MOTION_DITHER__
#define __INKPLATE_MOTION_DITHER__

// Include Arduino Header file.
#include <Arduino.h>

// Include dither kernel typedef and preddefined kernels.
#include "ditherKernels.h"

class Inkplate;

class ImageProcessing
{
    public:
        ImageProcessing();
        ~ImageProcessing();

        bool begin(Inkplate *_inkplate, uint16_t _displayWidth);
        void processImage(uint8_t *image, int16_t _x, int16_t _y, uint16_t _w, uint16_t _h, bool _ditheringEnabled, bool _colorInversion, const KernelElement *kernel, size_t kernel_size, uint8_t output_bit_depth);
        void toGrayscaleRow(uint8_t *_imageBuffer, uint16_t _imageWidth, uint8_t _redParameter, uint8_t _greenParameter, uint8_t _blueParameter);
        void invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth);
        void ditherImageRow(uint8_t *_currentRow, uint8_t *_nextRow, uint8_t *_rowAfterNext, int16_t _y, uint16_t width, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth);
        void writePixels(int16_t _x, int16_t _y, uint8_t *_imageBuffer, uint16_t _imageWidth);

    private:
        void freeResources();
        void moveBuffers(uint8_t **_currentRow, uint8_t **_nextRow, uint8_t **_rowAfterNext);

        Inkplate *_inkplatePtr = NULL;
        int _errorBufferSize = 0;
        uint16_t _displayW = 0;
        int16_t *_errorBuffer = NULL;
        int16_t *_nextErrorBuffer = NULL;
        int16_t *_afterNextErrorBuffer = NULL;
        uint8_t *_bufferedRowsArray = NULL;
        uint8_t *_bufferRows[3];
};

#endif