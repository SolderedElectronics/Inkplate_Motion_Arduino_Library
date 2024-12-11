// Include main header file for image processing.
#include "dither.h"

#include "InkplateMotion.h"

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
    _errorBufferSize = (_displayW * sizeof(uint16_t)) + 10;

    // Allocate the memory for all buffers.
    _errorBuffer = (int16_t*)malloc(_errorBufferSize);
    _nextErrorBuffer = (int16_t*)malloc(_errorBufferSize);

    // Max width = two times the screen width. Since is RGB = 24bits or 3 bytes, multiply by three.
    // Also three rows are buffered due Stucki dither, so multiply by three.
    _bufferedRowsArray = (uint8_t*)malloc(_displayW * sizeof(uint8_t) * 3 * 2 * 3);

    // Set the pointer array for easier access.
    _bufferRows[0] = _bufferedRowsArray;
    _bufferRows[1] = _bufferedRowsArray + (_displayW * sizeof(uint8_t) * 3 * 2);
    _bufferRows[2] = _bufferedRowsArray + (_displayW * sizeof(uint8_t) * 3 * 2 * 2);

    // Check for allocation.
    if ((_errorBuffer == NULL) || (_nextErrorBuffer == NULL) || (_bufferedRowsArray == NULL))
    {
        // Dang! Allocation has failed, abort everything!
        freeResources();

        // Go back with the error.
        return false;
    }

    // If everything went smooth, return true for success.
    return true;
}

void ImageProcessing::processImage(uint8_t *_imageFramebuffer, int16_t _x0, int16_t _y0, uint16_t _w, uint16_t _h, bool _ditheringEnabled, bool _colorInversion, const KernelElement *_ditherKernel, size_t _kernelDescriptorSize, uint8_t _outputBitDepth)
{
    // Check for the input parameters.
    if ((_imageFramebuffer == NULL) || (_w == 0) || (_h == 0)) return;

    // Constrain width of the image to max two times width of the screen.
    if (_w > (_displayW * 2)) _w = _displayW * 2;

    // Initialize dither buffers.
    memset(_errorBuffer, 0, _errorBufferSize);
    memset(_nextErrorBuffer, 0, _errorBufferSize);

    // Feed the data!
    // Copy one line into SRAM since it's faster to process image from SRAM than from SDRAM.
    memcpy(_bufferRows[0], _imageFramebuffer, _w * 3);
    memcpy(_bufferRows[1], _imageFramebuffer + ((1024 * 1) * 3), _w * 3);
    memcpy(_bufferRows[2], _imageFramebuffer + ((1024 * 2) * 3), _w * 3);

    this->toGrayscaleRow(_bufferRows[0], _w, 54, 183, 19);
    this->toGrayscaleRow(_bufferRows[1], _w, 54, 183, 19);
    this->toGrayscaleRow(_bufferRows[2], _w, 54, 183, 19);

    if (_colorInversion) this->invertColorsRow(_bufferRows[0], _w);
    if (_colorInversion) this->invertColorsRow(_bufferRows[1], _w);
    if (_colorInversion) this->invertColorsRow(_bufferRows[2], _w);

    // Process line-by-line since it's buffered and accessing SDRAM byte-by-byte is slow.
    for (int _y = 0; _y < _h; _y++)
    {
        // Conversion to the grayscale must be done, cannot be disabled!
        // Use BT.709 standard for RGB to grayscale conversion.
        // R = 0.2126 => 0.2126 * 256 = 54.4256 = 54
        // G = 0.7152 => 0.7152 * 256 = 183.0912 = 183
        // B = 0.0722 => 0.0722 * 256 = 18.4832 = 19
        // Tune if needed.
        this->toGrayscaleRow(_bufferRows[0], _w, 54, 183, 19);

        // Do the inversion if needed.
        if (_colorInversion) this->invertColorsRow(_bufferRows[0], _w);

        // Do the dither if needed.
        if (_ditheringEnabled)
        {
           // Update the buffers. Since there is a support for Stucki Dither that uses 3 rows, all of them needs to be updated.
           this->ditherImageRow(_bufferRows[0], _bufferRows[1], _bufferRows[2], _y, _w, _ditherKernel, _kernelDescriptorSize, _outputBitDepth);
        }

        // Push the pixels to the epaper main framebuffer.
        //this->writePixels(_x0, _y + _y0, _bufferRows[0], _w);

        this->moveBuffers(&_bufferRows[0], &_bufferRows[1], &_bufferRows[2]);
        memcpy(_bufferRows[2], _imageFramebuffer + ((1024 * (_y + 3)) * 3), _w * 3);
    }
}

void ImageProcessing::toGrayscaleRow(uint8_t *_imageBuffer, uint16_t _imageWidth, uint8_t _redParameter, uint8_t _greenParameter, uint8_t _blueParameter)
{
    for (int _x = 0; _x < _imageWidth; _x++)
    {
        // get the individual RGB colors.
        uint8_t _r = _imageBuffer[(_x * 3) + 2];
        uint8_t _g = _imageBuffer[(_x * 3) + 1];
        uint8_t _b = _imageBuffer[_x * 3];

        // Convert them into grayscale.
        uint8_t _pixel = (((uint32_t)(_redParameter) * _r) + ((uint32_t)(_greenParameter) * _g) + ((uint32_t)(_blueParameter) * _b)) >> 8;

        // Write into epaper framebuffer.
        //_inkplate->drawPixel(_x + x, _y + y, _pixel);
        //_framebufferHandler.framebuffer[(_x + (_framebufferHandler.fbWidth * _y))] = _pixel;
        _imageBuffer[_x] = _pixel;
    }
}

void ImageProcessing::invertColorsRow(uint8_t *_imageBuffer, uint16_t _imageWidth)
{
    for (int _xPixel = 0; _xPixel < _imageWidth; _xPixel++)
    {
        _imageBuffer[_xPixel] = ~_imageBuffer[_xPixel];
    }
}

void ImageProcessing::ditherImageRow(uint8_t *_currentRow, uint8_t *_nextRow, uint8_t *_rowAfterNext, int16_t _y, uint16_t width, const KernelElement *_ditherKernelParameters, size_t _ditherKernelParametersSize, uint8_t _bitDepth)
{
    // Initialize next error buffer.
    memset(_nextErrorBuffer, 0, _errorBufferSize);

    for (uint16_t _x = 0; _x < width; _x++)
    {
        // Adjust pixel value with propagated error
        int16_t _oldPixel = _currentRow[_x] + _errorBuffer[_x + 1];

        // Clamp to valid 8-bit range.
        if (_oldPixel < 0) _oldPixel = 0;
        if (_oldPixel > 255) _oldPixel = 255;

        // Check the bit depth.
        uint8_t _newPixel = 0;
        if (_bitDepth == 1)
        {
            // 1-bit output: threshold to black or white
            _newPixel = (_oldPixel >= 128) ? 255 : 0;
        }
        else if (_bitDepth == 4)
        {
            // 4-bit output: quantize to 0-15
            _newPixel = (_oldPixel * 15 + 127) / 255;
        }

        // Output the pixel
        _inkplatePtr->drawPixel(_x, _y, _newPixel); // Adjusted for x and y coordinates
        _currentRow[_x] = _newPixel;

        // Reverse quantization to calculate error
        int16_t quant_error = (_bitDepth == 1)
                                  ? _oldPixel - ((_newPixel == 255) ? 255 : 0)
                                  : _oldPixel - (_newPixel * 255 / 15);

        // Propagate error using the kernel
        for (size_t i = 0; i < _ditherKernelParametersSize; i++)
        {
            int16_t _newX = _x + _ditherKernelParameters[i].x_offset;
            if (_newX >= 0 && _newX < width)
            {
                if (_ditherKernelParameters[i].y_offset == 0)
                {
                    // Same row
                    _errorBuffer[_newX + 1] += (quant_error * _ditherKernelParameters[i].weight) / 16;
                }
                else if (_ditherKernelParameters[i].y_offset == 1 && _nextRow)
                {
                    // Next row
                    _nextRow[_newX] += (quant_error * _ditherKernelParameters[i].weight) / 16;
                }
                else if (_ditherKernelParameters[i].y_offset == 2 && _rowAfterNext)
                {
                    // Row after next
                    _rowAfterNext[_newX] += (quant_error * _ditherKernelParameters[i].weight) / 16;
                }
            }
        }
    }

    // Update the error buffer for the next row
    memcpy(_errorBuffer, _nextErrorBuffer, _errorBufferSize);
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
    *_currentRow = *_nextRow;
    *_nextRow = *_rowAfterNext;
}

void ImageProcessing::freeResources()
{
    if (_nextErrorBuffer) free(_nextErrorBuffer);
    if (_errorBuffer) free(_errorBuffer);
    if (_bufferedRowsArray) free(_bufferedRowsArray);
}