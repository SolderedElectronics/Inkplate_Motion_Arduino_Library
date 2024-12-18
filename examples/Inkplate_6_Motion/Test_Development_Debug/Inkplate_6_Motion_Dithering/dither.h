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
        void processImage(uint8_t *_imageBuffer, int16_t _x0, int16_t _y0, uint16_t _width, uint16_t _height, bool _ditheringEnabled, bool _colorInversion, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth);
        void toGrayscaleRow(uint8_t *_imageBuffer, uint16_t _imageWidth, uint8_t _redParameter, uint8_t _greenParameter, uint8_t _blueParameter);
        void invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth);
        void ditherImageRow(uint8_t *_currentRow, uint8_t *_nextRow, uint8_t *_rowAfterNext, int16_t _y0, uint16_t _width, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth);
        void writePixels(int16_t _x0, int16_t _y0, uint8_t *_imageBuffer, uint16_t _imageWidth);

    private:
        void freeResources();
        void moveBuffers(uint8_t **_currentRow, uint8_t **_nextRow, uint8_t **_rowAfterNext);
        void prepare(uint8_t *_imageFramebuffer, uint16_t _width,  bool _ditheringEnabled, bool _colorInversion,  const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize);

        Inkplate *_inkplatePtr = NULL;
        uint16_t _displayW = 0;

        // Error buffers related.
        int _errorBufferSize = 0;
        int16_t *_errorBuffer = NULL;
        int16_t *_nextErrorBuffer = NULL;
        int16_t *_afterNextErrorBuffer = NULL;
        
        // Pixel buffers.
        //uint8_t *_bufferedRowsArray = NULL;
        //uint8_t *_bufferRows[3];

        // Buffer for precalculate d weight factors.
        int16_t *_ditherWeightFactors = NULL;
};

#endif