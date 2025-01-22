#ifndef __INKPLATE_MOTION_DITHER__
#define __INKPLATE_MOTION_DITHER__

// Include Arduino Header file.
#include <Arduino.h>

// Include dither kernel typedef and preddefined kernels.
#include "ditherKernels.h"

// Inkplate class forward declaration.
class Inkplate;

// Class for Inkplate image processing - it adjusts image to the screen (24RGB to 8bit RGB Grayscale, dithering, inversion etc).
class ImageProcessing
{
    public:
        // Class constructor.
        ImageProcessing();
        // Class destructor - releases allocated memory.
        ~ImageProcessing();

        // Initialization of the library. Returns fail is memory allocation failed or ambiguous input parameters.
        bool begin(Inkplate *_inkplate, uint16_t _displayWidth);

        // Main function that does the whole image processing - it does this row-by-row due SDRAM and buffering. It pre-buffers three rows in advance.
        void processImage(uint8_t *_imageBuffer, int16_t _x0, int16_t _y0, uint16_t _width, uint16_t _height, bool _ditheringEnabled, bool _colorInversion, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth);

    private:
        void toGrayscaleRow(uint8_t *_imageBuffer, uint16_t _imageWidth, uint8_t _redParameter = 54, uint8_t _greenParameter = 183, uint8_t _blueParameter = 19);
        void invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth);
        void ditherImageRow(uint8_t *_currentRow, uint8_t *_nextRow, uint8_t *_rowAfterNext, int16_t _y0, uint16_t _width, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth);
        void writePixels(int16_t _x0, int16_t _y0, uint8_t *_imageBuffer, uint16_t _imageWidth);
        void freeResources();
        void moveBuffers(uint8_t **_currentRow, uint8_t **_nextRow, uint8_t **_rowAfterNext);
        void prepare(uint8_t *_imageFramebuffer, uint16_t _width,  bool _ditheringEnabled, bool _colorInversion,  const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize);

        // Internal pointer of the inkplate library object.
        Inkplate *_inkplatePtr = NULL;
        // Real width of the screen (must exclude rotation).
        uint16_t _displayW = 0;

        // Error buffers related.
        int _errorBufferSize = 0;
        int16_t *_errorBuffer = NULL;
        int16_t *_nextErrorBuffer = NULL;
        int16_t *_afterNextErrorBuffer = NULL;

        // Buffer for precalculate d weight factors.
        int16_t *_ditherWeightFactors = NULL;
};

#endif