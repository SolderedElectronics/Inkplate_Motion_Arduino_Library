// Include main header file for image processing.
#include "dither.h"

#include "InkplateMotion.h"

/* TODO LIST:
 * - [Done] Use 8 bit mode
 * - [Done] Optimise the code - procesing under 200ms would be great
 *      - Can't be done, pixel write is slow as well as the dithering.
 * - [Done] Use DMA for SDRAM buffering.
 * - [Done] Force function for dithering to output data on the current row that is not used anymore after dither has been done.
 * - [Done] Do a general overview and checks
 * - [Done] Do a DoxyGen
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

/**
 * @brief   Initializes image processing library.
 * 
 * @param   Inkplate *_inkplate
 *          Inkplate object - used for drawPixel and DMA buffers.
 * @param   uint16_t _displayWidth
 *          Real screen width in pixels (without rotation). 
 * @return  bool
 *          true - Initialization successful.
 *          false - Initialization failed. Check memory or input parameters.
 */
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

/**
 * @brief   Main processing method for the provided framebuffer.
 * 
 * @param   uint8_t *_imageBuffer
 *          Pointer to the address where image is stored. Only works on RGB888 (24bit RGB).
 * @param   int16_t _x0
 *          X position where to draw image on the screen.
 * @param   int16_t _y0
 *          Y position where to draw image on the screen.
 * @param   uint16_t _width
 *          Width of the image stored in the framebuffer (in pixels).
 * @param   uint16_t _height
 *          Height of the image stored in the framebuffer (in pixels).
 * @param   bool _ditheringEnabled
 *          Switch for disable/enable image dithering.
 * @param   bool _colorInversion
 *          Switch for the disable/enable pixel color inversion.
 * @param   const KernelElement *_ditherKernelParameters
 *          Pointer to the dither kernel parameters. NULL if dithering is not used.
 *          Provided Kernels can be used or custom ones.
 * @param   size_t _ditherKernelParametersSize
 *          Dither kernels size in bytes.
 * @param   uint8_t _bitDepth
 *          Output bit depth - 4 for 4 bit mode, 1 for 1 bit mode.
 *          Other modes are not supported.
 * 
 * @note    Processing is done row-by-row due SDRAM buffering.
 *          
 */
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
        this->toGrayscaleRow((uint8_t*)(_inkplatePtr->_dmaBuffer[2]), _width);

        // Do the inversion if needed.
        if (_colorInversion) this->invertColorsRow((uint8_t*)(_inkplatePtr->_dmaBuffer[2]), _width);
    }

    // Free allocated memory for dither weight factors.
    if (_ditheringEnabled) free(_ditherWeightFactors);
}

/**
 * @brief   Process image - convert it into the 8 bit grayscale. Use the same input buffer for the 
 *          output (overwritte old/original data).
 * 
 * @param   uint8_t *_imageBuffer
 *          Pointer to the address where image is stored. Only works on RGB888 (24bit RGB).
 * @param   uint16_t _imageWidth
 *          Size of the image in the buffer (in pixels).
 * @param   uint8_t _redParameter
 *          Parameter for the red to grayscale conversion.
 * @param   uint8_t _greenParameter
 *          Parameter for the green to grayscale conversion.
 * @param   uint8_t _blueParameter
 *          Parameter for the blue to grayscale conversion.
 * 
 * @note    Conversion to the grayscale must be done, cannot be disabled in the image processing.
 *          By default, it's used BT.709 standard for RGB to grayscale conversion.
 *          R = 0.2126 => 0.2126 * 256 = 54.4256 = 54
 *          G = 0.7152 => 0.7152 * 256 = 183.0912 = 183
 *          B = 0.0722 => 0.0722 * 256 = 18.4832 = 19
 */
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

/**
 * @brief   Goes though whole framebuffer and inverts color (by simply inverting the byte).
 *          Works both on 4 bit and 1 bit mode.
 * 
 * @param   uint8_t *_imageBuffer
 *          Pointer to the framebuffer needs to be processed.
 * @param   uint16_t _imageWidth
 *          Width of the image stored in framebuffer.
 *          
 */
void ImageProcessing::invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth)
{
    // Invert the colors by inverting whole 8 bit color byte.
    for (int _xPixel = 0; _xPixel < _imageWidth; _xPixel++)
    {
        _imageBuffer[_xPixel] = ~_imageBuffer[_xPixel];
    }
}

/**
 * @brief   Method dithers one row of the provided 
 *          
 * @param   uint8_t *_currentRow
 *          Pointer to the address of the buffer for the current processed row.
 * @param   uint8_t *_nextRow
 *          Pointer to the address of the buffer for the next processed row.
 * @param   uint8_t *_rowAfterNext
 *          Pointer to the address of the buffer for the after next processed row.
 * @param   int16_t _y0
 *          Y position where the image should be drawn on screen.
 * @param   uint16_t _width
 *          Width of the image provided in the buffers (in pixels).
 * @param   const KernelElement *_ditherKernelParameters
 *          Pointer to the dither kernel parameters. NULL if dithering is not used.
 *          Provided Kernels can be used or custom ones.
 * @param   size_t _ditherKernelParametersSize
 *          Dither kernels size in bytes.
 * @param   uint8_t_bitDepth
 *          Output bit depth - 4 for 4 bit mode, 1 for 1 bit mode.
 *          Other modes are not supported.
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

/**
 * @brief   Method writes the pixels into the epaper/Inkplate main framebuffer.
 * 
 * @param   int16_t _x0
 *          X coridinate where to start writing the image.
 * @param   int16_t _y0
 *          Y coridinate where to start writing the image.
 * @param   uint8_t *_imageBuffer
 *          Pointer to the framebuffer needs to be processed.
 * @param   uint16_t _imageWidth
 *          Width of the image stored in framebuffer.
 */
void ImageProcessing::writePixels(int16_t _x0, int16_t _y0, uint8_t *_imageBuffer, uint16_t _imageWidth)
{
    for (int _xPixel = 0; _xPixel < _imageWidth; _xPixel++)
    {
        _inkplatePtr->drawPixel(_x0 + _xPixel, _y0, _imageBuffer[_xPixel] >> 4);
    }
}

/**
 * @brief   Moves the buffers around. Moves them forward and loops back around.
 *          It goes [1][2][3] -> [2][3][1].
 * 
 * @param   uint8_t **_currentRow
 *          Pointer to the first buffer.
 * @param   uint8_t **_nextRow
 *          Pointer to the second buffer.
 * @param   uint8_t **_rowAfterNext
 *          Pointer to the three buffer.
 *          
 */
void ImageProcessing::moveBuffers(uint8_t **_currentRow, uint8_t **_nextRow, uint8_t **_rowAfterNext)
{
    // Move the buffers([1][2][3] -> [2][3][1]).
    uint8_t *_temp = *_currentRow;
    *_currentRow = *_nextRow;
    *_nextRow = *_rowAfterNext;
    *_rowAfterNext = _temp;
}

/**
 * @brief   Releases the used resources.
 * 
 */
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
    _ditherWeightFactors = NULL;
}

/**
 * @brief   Method is used to prepare the buffers for the image processing.
 *          It preloads, decodes first three rows, precalculates the dither parameters and clears the buffers.
 * 
 * @param   uint8_t *_imageFramebuffer
 *          Pointer to the framebuffer needs to be processed.
 * @param   uint16_t _w
 *          Size of the image in the buffer that needs to be processes (in pixels).
 * @param   bool _ditheringEnabled
 *          Switch for disable/enable image dithering.
 * @param   bool _colorInversion
 *          Switch for the disable/enable pixel color inversion.
 * @param   const KernelElement *_ditherKernelParameters
 *          Pointer to the dither kernel parameters. NULL if dithering is not used.
 *          Provided Kernels can be used or custom ones.
 * @param   size_t_ditherKernelParametersSize
 *          Dither kernels size in bytes.
 */
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