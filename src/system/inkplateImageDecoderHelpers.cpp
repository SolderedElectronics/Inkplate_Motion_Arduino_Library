/**
 **************************************************
 *
 * @file        inkplateImageDecoderHelpers.cpp
 * @brief       Source file for the image decoder helper
 *              functions. These functions are common for all boards.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Include main header file.
#include "inkplateImageDecoderHelpers.h"

/**
 * @brief   Core function for decoding BMP image. It's should be iniversal for all Inkplate
 *          boards.
 * 
 * @param   BmpDecodeHandle *_bmpDecoder
 *          Pointer to the BMP decoder handle.
 *          
 * @param   InkplateImageDecodeErrors *_decodeError
 *          Handler for the ImageDecoder class errors.
 * @return  bool
 *          true - Image decode succ. Decoded image is stored in temp. framebuffer for decoded image.
 *          false - Image decode failed.
 */
bool inkplateImageDecodeHelpersBmp(BmpDecodeHandle *_bmpDecoder, InkplateImageDecodeErrors *_decodeError)
{
    // Check for the wrong input parameters.
    if ((_bmpDecoder == NULL) || (_decodeError == NULL))
    {
        // Since we can't be sure what is missing, it could be _decodeError, we can't
        // save the decoder error.
        return false;
    }

    // Try to read if the file is vaild.
    if (!bmpDecodeVaildFile(_bmpDecoder))
    {
        // Set error while drawing image.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

        // Return!
        return false;
    }

    // Try to decode a BMP header.
    if (!bmpDecodeProcessHeader(_bmpDecoder))
    {
        // Set error while drawing image.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

        // Return!
        return false;
    }

    // Check if the BMP is a valid BMP for the decoder.
    if (!bmpDecodeVaildBMP(_bmpDecoder))
    {
        // Set error while drawing image.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

        // Return!
        return false;
    }

    if (!bmpDecodeProcessBmp(_bmpDecoder))
    {
        // Set error while drawing image.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT;

        // Return!
        return false;
    }

    // If decode process was ok, return true for success.
    return true;
}

/**
 * @brief   Core function for decoding JPG image. It's should be iniversal for all Inkplate
 *          boards.
 * 
 * @param   JDEC *_jpgDecoder
 *          Pointer to the JPG Decoder handler.
 * @param   size_t (*_inFunc)(JDEC *, uint8_t *, size_t)
 *          Input callback function for loading the data into the JPG decoder.
 * @param   int (*_outFunc)(JDEC *, void *, JRECT *)
 *          Output callback function for drawing pixel of decoded image into the framebuffer.
 * @param   InkplateImageDecodeErrors *_decodeError
 *          Pointer to the decoder error handler of the ImageDecoder class.
 * @param   void *_sessionHandler
 *          Session handler - Callback specific struct/typedef to access file, framebuffer, other classes etc from
 *          the callback itself.
 * @return  bool
 *          true - JPG image decoded succ.
 *          false -  JPG image decode failed.
 */
bool inkplateImageDecodeHelpersJpg(JDEC *_jpgDecoder, size_t (*_inFunc)(JDEC *, uint8_t *, size_t), int (*_outFunc)(JDEC *, void *, JRECT *), InkplateImageDecodeErrors *_decodeError, void *_sessionHandler)
{
    // Check for the wrong input parameters.
    if ((_jpgDecoder == NULL) || (_decodeError == NULL))
    {
        // Since we can't be sure what is missing, it could be _decodeError, we can't
        // save the decoder error.
        return false;
    }

    // Check for the callbacks and other input parameters.
    if ((_inFunc == NULL) || (_outFunc) || (_sessionHandler == NULL))
    {
        // Set the error.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;

        // Return false for fail.
        return false;
    }

    // Allocate the memory for the JPG decoder.
    const size_t _workingBufferSize = 4096;
    void *_workingBuffer;

    // Allocate the memory for the buffer.
    _workingBuffer = (void*)malloc(_workingBufferSize);
    if (_workingBuffer == NULL)
    {
        // Set the error.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_NO_MEMORY;

        // Return fail.
        return false;
    }

    // Fill the decoder with parameters.
    JRESULT _result = jd_prepare(_jpgDecoder, _inFunc, _workingBuffer, _workingBufferSize, _sessionHandler);

    // Check if JPG decoder prepare is ok. If not, return error.
    if (_result == JDR_OK)
    {
        // Set output callback and decode the image!
        _result = jd_decomp(_jpgDecoder, _outFunc, 0);

        // If failed, free memory and return fail.
        if (_result != JDR_OK)
        {
            // Free allocated memory.
            free(_workingBuffer);
        
            // Set the error.
            (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_JPG_DECODER_FAULT;

            // Return fail.
            return false;
        }
    }
    else
    {
        // Free allocated memory.
        free(_workingBuffer);

        // Set the error.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_NO_MEMORY;

        // Return fail.
        return false;
    }

    // Free up the memory.
    free(_workingBuffer);

    // If decode process was ok, return true for success.
    return true;
}

/**
 * @brief   Core function for decoding BMP image. It's should be iniversal for all Inkplate
 *          boards.
 * 
 * @param   pngle_t *_pngDecoder
 *          Pointer to the PNG Decoder handler.
 * @param   bool (*_inFunc)(pngle_t *_pngle)
 *          Input callback function for loading the data into the PNG decoder.
 * @param   void (*_outFunc)(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
 *          Output callback function for drawing pixel of decoded image into the framebuffer.
 * @param   _imgW
 *          Pointer to the variable to store image width.
 * @param   _imgH
 *          Pointer to the variable to store image height.
 * @param   _decodeError
 *          Pointer to the decoder error handler of the ImageDecoder class.
 * @param   _sessionHandler
 *          Session handler - Callback specific struct/typedef to access file, framebuffer, other classes etc from
 *          the callback itself.
 * @return  bool
 *          true - Image decoded succ.
 *          false - Image decode failed.
 */
bool inkplateImageDecodeHelpersPng(pngle_t *_pngDecoder, bool (*_inFunc)(pngle_t *_pngle), void (*_outFunc)(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]), int *_imgW, int *_imgH, InkplateImageDecodeErrors *_decodeError, void *_sessionHandler)
{
    // Check for the wrong input parameters.
    if ((_pngDecoder == NULL) || (_decodeError == NULL))
    {
        // Since we can't be sure what is missing, it could be _decodeError, we can't
        // save the decoder error.
        return false;
    }

    // Check for the callbacks and other input parameters.
    if ((_inFunc == NULL) || (_outFunc) || (_sessionHandler == NULL))
    {
        // Set the error.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM;

        // Return false for fail.
        return false;
    }
    
    // Decode it chunk-by-chunk.
    // Allocate new PNG decoder.
    _pngDecoder = pngle_new();

    // Add session handler for the framebuffer access.
    pngle_set_user_data(_pngDecoder, _sessionHandler);

    // Set the callback for decoder.
    pngle_set_draw_callback(_pngDecoder, _outFunc);

    // Do a callback for the input data feed!
    if (!_inFunc(_pngDecoder))
    {
        // Decoder failed to load image, load error status.
        (*_decodeError) = INKPLATE_IMAGE_DECODE_ERR_PNG_DECODER_FAULT;

        // Free up the memory.
        pngle_destroy(_pngDecoder);

        // Return false as error.
        return false;
    }

    // Check image size and constrain it.
    (*_imgW) = pngle_get_width(_pngDecoder);
    (*_imgH) = pngle_get_height(_pngDecoder);

    // Free up the memory after succ decode.
    pngle_destroy(_pngDecoder);

    // Return true for successfull image decode.
    return true;
}

/**
 * @brief   Function used for detecting image format (BMP, JPG or PNG) by
 *          using an extension or file format signature.
 * 
 * @param   char *_filename
 *          Pointer to the image filename or path.
 *          
 * @param   void *_bytes
 *          Pointer to the 10 byte sample of the file. Must be from the start
 *          of the file 
 *          
 * @return  enum InkplateImageDecodeFormat
 */
enum InkplateImageDecodeFormat inkplateImageDecodeHelpersDetectImageFormat(char *_filename, void *_bytes)
{
    // First, try to get the image format by it's extension.
    // Firstly try to find the extension itself. It is at the end of the filename.
    // So, first get the lenght of the filename. But check if this needs to be skipped.
    if (_filename != NULL)
    {
        int _filenameLen = strlen(_filename);

        // Now extract last three letters.
        char _extension[5];
        for (int i = 0; i < 4; i++)
        {
            _extension[i] = _filename[_filenameLen - 4 + i];
        }
        // Add nul-terminating char.
        _extension[4] = '\0';

        // Convert it to uppercase.
        for (int i = 0; i < strlen(_extension); i++)
        {
            _extension[i] = toupper(_extension[i]);
        }

        if (strstr(_extension, ".BMP"))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_BMP;
        }
        else if (strstr(_extension, ".JPG"))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_JPG;
        }
        else if (strstr(_extension, ".PNG"))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_PNG;
        }
    }

    // First check if the file format signature needs to be skipped.
    if (_bytes != NULL)
    {
        // If extension method failed, try with reading format signature inside the header.
        if (inkplateImageDecodeHelpersCheckHeaders((uint8_t*)_bytes, (uint8_t*)_helpersBmpSignature))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_BMP;
        }
        else if (inkplateImageDecodeHelpersCheckHeaders((uint8_t*)_bytes, (uint8_t*)_helpersJpgSignature))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_JPG;
        }
        else if (inkplateImageDecodeHelpersCheckHeaders((uint8_t*)_bytes, (uint8_t*)_helpersPngSignature))
        {
            return INKPLATE_IMAGE_DECODE_FORMAT_PNG;
        }
    }

    // If this also failed, then this may not be a vaild image format or both of the
    // pointers are NULL for some reason.
    return INKPLATE_IMAGE_DECODE_FORMAT_ERR;
}

/**
 * @brief   Function compares header against known file format
 *          signature. On first match it returns detected file format.
 * 
 * @param   void *_dataPtr
 *          Sample of the unknown file format (header, first 10 bytes).
 * @param   void *_headerSignature
 *          Array of the current selected file format signature. First element in this
 *          array sets how long the signature is.
 * @return  bool
 *          true - Set signature has match.
 *          false - No match, try next one.
 */
bool inkplateImageDecodeHelpersCheckHeaders(void *_dataPtr, void *_headerSignature)
{
    // Check for invalid input.
    if ((_dataPtr == NULL) || (_headerSignature == NULL)) return false;

    // Convert void pinter into bytes.
    uint8_t *_array = (uint8_t*)_headerSignature;

    // Save the number of signature element - first element of the signature array.
    uint8_t _n = _array[0];
    _array++;
    
    // Go trough the byte array.
    for (int i = 0; i < _n; i++)
    {
        if (((uint8_t*)_dataPtr)[i] != _array[i]) return false;
    }
    
    // If no brake statement has occured, header signature has match.
    return true;
}

/**
 * @brief   Check if the path is actually web path instead of microSD path.
 * 
 * @param   char *_path
 *          Pointer to the filename or path.
 * @return  bool
 *          true - Path is web location.
 *          false - Path is local (microSD card).
 */
bool inkplateImageDecodeHelpersIsWebPath(char *_path)
{
    // Copy first 20 letters of the path locally.
    char _upperCasePath[21];
    
    // Convert them to the uppercase.
    for (int i = 0; i < 20; i++)
    {
        _upperCasePath[i] = toupper(_path[i]);
    }
    
    // Add null-ternimating char.
    _upperCasePath[20] = '\0';
    
    // Compare agaist HTTP, HTTTPS, FTP. It match has been found, return true.
    if (strstr(_upperCasePath, "HTTP://") || strstr(_upperCasePath, "HTTPS://") || strstr(_upperCasePath, "FTP://"))
    {
        return true;
    }
    
    // Try one more time to check if the path is IP Address.
    int _dummy;
    if (sscanf(_upperCasePath, "%d.%d.%d.%d", &_dummy, &_dummy, &_dummy, &_dummy) == 4)
    {
        return true;
    }
    
    // More checks can be added, but also manual override can be used instead.
    
    // If nothing has been found, return false.
    return false;
}