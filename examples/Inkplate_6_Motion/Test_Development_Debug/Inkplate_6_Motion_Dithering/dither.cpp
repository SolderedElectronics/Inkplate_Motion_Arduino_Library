// Include main header file for image processing.
#include "dither.h"

#include "InkplateMotion.h"

/* TODO LIST:
 * - [Done] Use 8 bit mode
 * - Optimise the code - procesing under 200ms would be great - can't be done, pixel write is slow as well as the dithering.
 * - [Done] Use DMA for SDRAM buffering.
 * - [Done] Force function for dithering to output data on the current row that is not used anymore after dither has been done.
 * - [Done] Do a general overview and checks
 * - Do a DoxyGen
 * - [Done] Test on different modes - modes should be automatically selected.
 * - [Done] Check allocations in the begin!
 * - Check if code can be more readable.
*/

/**
 * @brief Construct a new Image Processing object
 * 
 */
ImageProcessing::ImageProcessing()
{
    // Empty.
}

/**
 * @brief Destroy the Image Processing object. Free up allocated memory!
 * 
 */
ImageProcessing::~ImageProcessing()
{
    // Free allocated memory if allocated.
    this->freeResources();
}

bool ImageProcessing::begin(Inkplate *_inkplate, uint16_t _displayWidth)
{
    // Check for the Inkplate object pointer.
    if (_inkplate == NULL) return false;

    // Copy object address locally.
    _inkplatePtr = _inkplate;

    // Copy screen width locally.
    _displayW = _displayWidth;

    // Calculate the size of the error buffers.
    _errorBufferSize = (_displayW + 10) * sizeof(uint16_t);

    // Allocate the memory for all buffers.
    _errorBuffer = (int16_t*)malloc(_errorBufferSize);
    _nextErrorBuffer = (int16_t*)malloc(_errorBufferSize);
    _afterNextErrorBuffer = (int16_t*)malloc(_errorBufferSize);

    // Since is RGB = 24bits or 3 bytes, multiply by three.
    // Also three rows are buffered due Stucki dither, so multiply by three.
    //_bufferedRowsArray = (uint8_t*)malloc(_displayW * sizeof(uint8_t) * 3 * 3);

    // Check for allocation.
    if ((_errorBuffer == NULL) || (_nextErrorBuffer == NULL) || (_afterNextErrorBuffer == NULL))
    {
        // Dang! Allocation has failed, abort everything!
        freeResources();

        // Go back with the error.
        return false;
    }

    // If everything went smooth, return true for success.
    return true;
}

void ImageProcessing::processImage(uint8_t *_imageBuffer, int16_t _x0, int16_t _y0, uint16_t _width, uint16_t _height, bool _ditheringEnabled, bool _colorInversion, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth)
{
    // Check for the input parameters.
    if ((_imageBuffer == NULL) || (_width == 0) || (_height == 0)) return;

    // Constrain width of the image to the width of the screen.
    if (_width > _displayW) _width = _displayW;

    // Prepare buffers for image processing.
    this->prepare(_imageBuffer, _width, _ditheringEnabled, _colorInversion, _ditherKernelParameters, _ditherKernelParametersSize);

    // Process line-by-line since it's buffered and accessing SDRAM byte-by-byte is slow.
    for (int _y = 0; _y < _height; _y++)
    {
        // Do the dither if needed.
        if ((_ditheringEnabled) && (_ditherKernelParameters) && (_ditherKernelParametersSize > 0))
        {
           // Update the buffers. Since there is a support for Stucki Dither that uses 3 rows, all of them needs to be updated.
           this->ditherImageRow((uint8_t*)(_inkplatePtr->_dmaBuffer[0]), (uint8_t*)(_inkplatePtr->_dmaBuffer[1]), (uint8_t*)(_inkplatePtr->_dmaBuffer[2]), _y, _width, _ditherKernelParameters, _ditherKernelParametersSize, _bitDepth);
        }

        // Push the pixels to the epaper main framebuffer.
        this->writePixels(_x0, _y + _y0, (uint8_t*)(_inkplatePtr->_dmaBuffer[0]), _width);

        // Update the buffers!
        this->moveBuffers(((uint8_t**)&(_inkplatePtr->_dmaBuffer[0])), ((uint8_t**)&(_inkplatePtr->_dmaBuffer[1])), ((uint8_t**)&(_inkplatePtr->_dmaBuffer[2])));

        // Copy new data with DMA!
        HAL_MDMA_Start_IT(stm32FmcGetSdramMdmaInstance(), (uint32_t)_imageBuffer + ((1024 * (_y + 3)) * 3), (uint32_t)_inkplatePtr->_dmaBuffer[2], MULTIPLE_OF_4(_width * 3), 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();

        // Conversion to the grayscale must be done, cannot be disabled!
        // Use BT.709 standard for RGB to grayscale conversion.
        // R = 0.2126 => 0.2126 * 256 = 54.4256 = 54
        // G = 0.7152 => 0.7152 * 256 = 183.0912 = 183
        // B = 0.0722 => 0.0722 * 256 = 18.4832 = 19
        // Tune if needed.
        this->toGrayscaleRow((uint8_t*)(_inkplatePtr->_dmaBuffer[2]), _width, 54, 183, 19);

        // Do the inversion if needed.
        if (_colorInversion) this->invertColorsRow((uint8_t*)(_inkplatePtr->_dmaBuffer[2]), _width);
    }

    // Free allocated memory for dither weight factors.
    if (_ditheringEnabled) free(_ditherWeightFactors);
}

void ImageProcessing::toGrayscaleRow(uint8_t *_imageBuffer, uint16_t _imageWidth, uint8_t _redParameter, uint8_t _greenParameter, uint8_t _blueParameter)
{
    for (int _x = 0; _x < _imageWidth; _x++)
    {
        // Get the individual RGB colors.
        uint8_t _r = _imageBuffer[(_x * 3) + 2];
        uint8_t _g = _imageBuffer[(_x * 3) + 1];
        uint8_t _b = _imageBuffer[_x * 3];

        // Convert them into grayscale.
        uint8_t _pixel = (((uint32_t)(_redParameter) * _r) + ((uint32_t)(_greenParameter) * _g) + ((uint32_t)(_blueParameter) * _b)) >> 8;

        // Write into buffer.
        _imageBuffer[_x] = _pixel;
    }
}

void ImageProcessing::invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth)
{
    // Invert the colors by inverting whole 8 bit color byte.
    for (int _xPixel = 0; _xPixel < _imageWidth; _xPixel++)
    {
        _imageBuffer[_xPixel] = ~_imageBuffer[_xPixel];
    }
}

/**
 * @brief Optimize Error Propagation in Dithering
 *        Precompute error distribution weights and reduce unnecessary branching.
 */
void ImageProcessing::ditherImageRow(uint8_t *_currentRow, uint8_t *_nextRow, uint8_t *_rowAfterNext, int16_t _y0, uint16_t _width, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth)
{
    // Precompute constants based on bit depth.
    int16_t _quantErrorFactor = (_bitDepth == 1) ? 255 : 255 >> 4; // Divide by 16

    for (uint16_t _x = 0; _x < _width; _x++)
    {
        // Adjust pixel value with propagated error.
        int16_t _oldPixel = _currentRow[_x] + _errorBuffer[_x + 1];

        // Clamp to valid 8-bit range.
        _oldPixel = (_oldPixel < 0) ? 0 : (_oldPixel > 255) ? 255 : _oldPixel;

        uint8_t _newPixel;
        if (_bitDepth == 1)
        {
            // 1-bit output: threshold to black or white.
            _newPixel = (_oldPixel >= 128) ? 0 : 255;
        }
        else if (_bitDepth == 4)
        {
            // 4-bit output: quantize to 0-15.
            _newPixel = (_oldPixel * 15 + 127) >> 8; // Divide by 256
        }
        else
        {
            // Unsupported bit depth.
            _newPixel = 0;
        }

        // Output the pixel and move it by four bits since everything is processed at 8 bits.
        _currentRow[_x] = _newPixel << 4;

        // Reverse quantization to calculate error.
        int16_t _quantError = (_bitDepth == 1)
                                  ? _oldPixel - ((_newPixel == 255) ? 0 : 255)
                                  : _oldPixel - (_newPixel << 4); // Multiply by 16

        // Propagate error using the kernel.
        for (size_t _i = 0; _i < _ditherKernelParametersSize; _i++)
        {
            int16_t _newX = _x + _ditherKernelParameters[_i].x_offset;
            int16_t _newY = _ditherKernelParameters[_i].y_offset; // y_offset applies to current row.

            if (_newX >= 0 && _newX < _width)
            {
                int16_t *_targetBuffer = nullptr;

                // Determine which buffer to propagate error to (same row, next row, or after next row).
                if (_newY == 0)
                {
                    _targetBuffer = _errorBuffer; // Same row
                }
                else if (_newY == 1 && _nextRow)
                {
                    _targetBuffer = _nextErrorBuffer; // Next row
                }
                else if (_newY == 2 && _rowAfterNext)
                {
                    _targetBuffer = _afterNextErrorBuffer; // After-next row
                }

                // Propagate error to the appropriate buffer if valid.
                if (_targetBuffer)
                {
                    _targetBuffer[_newX + 1] += (_quantError * _ditherWeightFactors[_i]) >> 4; // Divide by 16
                }
            }
        }
    }

    // Swap error buffers for the next iteration.
    int16_t *_temp = _errorBuffer;
    _errorBuffer = _nextErrorBuffer;
    _nextErrorBuffer = _afterNextErrorBuffer;
    _afterNextErrorBuffer = _temp;

    // Clear the next and after-next error buffers.
    memset(_nextErrorBuffer, 0, _errorBufferSize);
    memset(_afterNextErrorBuffer, 0, _errorBufferSize);
}

void ImageProcessing::writePixels(int16_t _x0, int16_t _y0, uint8_t *_imageBuffer, uint16_t _imageWidth)
{
    for (int _xPixel = 0; _xPixel < _imageWidth; _xPixel++)
    {
        _inkplatePtr->drawPixel(_x0 + _xPixel, _y0, _imageBuffer[_xPixel] >> 4);
    }
}

void ImageProcessing::moveBuffers(uint8_t **_currentRow, uint8_t **_nextRow, uint8_t **_rowAfterNext)
{
    // Move the buffers([1][2][3] -> [2][3][1]).
    uint8_t *_temp = *_currentRow;
    *_currentRow = *_nextRow;
    *_nextRow = *_rowAfterNext;
    *_rowAfterNext = _temp;
}

void ImageProcessing::freeResources()
{
    // Free error buffers.
    if (_nextErrorBuffer) free(_nextErrorBuffer);
    if (_errorBuffer) free(_errorBuffer);
    if (_afterNextErrorBuffer) free(_afterNextErrorBuffer);

    // Free precalculated Weight factors.
    if (_ditherWeightFactors) free(_ditherWeightFactors);

    // Set every pointer to NULL.    
    _errorBuffer = NULL;
    _nextErrorBuffer = NULL;
    _afterNextErrorBuffer = NULL;
    //_bufferedRowsArray = NULL;
    _ditherWeightFactors = NULL;
}

void ImageProcessing::prepare(uint8_t *_imageFramebuffer, uint16_t _w,  bool _ditheringEnabled, bool _colorInversion,  const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize)
{
    // Initialize dither buffers.
    memset(_errorBuffer, 0, _errorBufferSize);
    memset(_nextErrorBuffer, 0, _errorBufferSize);
    memset(_afterNextErrorBuffer, 0, _errorBufferSize);

    // Feed the data!
    // Copy one line into SRAM since it's faster to process image from SRAM than from SDRAM.
    for (int i = 0; i < 3; i++)
    {
        // Use DMA and DMA buffers to copy pixels from SDRAM into SRAM.
        HAL_MDMA_Start_IT(stm32FmcGetSdramMdmaInstance(), (uint32_t)_imageFramebuffer + ((1024 * (i + 3)) * 3), (uint32_t)_inkplatePtr->_dmaBuffer[i], MULTIPLE_OF_4(_w * 3), 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();

        // Pre-process these three rows.
        this->toGrayscaleRow((uint8_t*)(_inkplatePtr->_dmaBuffer[i]), _w, 54, 183, 19);
        if (_colorInversion) this->invertColorsRow((uint8_t*)(_inkplatePtr->_dmaBuffer[i]), _w);
    }

    // Precompute weight factors for the dithering kernel.
    if ((_ditheringEnabled) && (_ditherKernelParameters) && (_ditherKernelParametersSize > 0))
    {
        _ditherWeightFactors = (int16_t*)malloc(sizeof(int16_t) * _ditherKernelParametersSize);
        if (_ditherWeightFactors == NULL) return;
        for (size_t i = 0; i < _ditherKernelParametersSize; i++)
        {
            _ditherWeightFactors[i] = _ditherKernelParameters[i].weight;
        }
    }
}