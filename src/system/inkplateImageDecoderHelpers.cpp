// Include main header file.
#include "inkplateImageDecoderHelpers.h"

/**
 * @brief   Core function for decoding BMP image. It's should be iniversal for all Inkplate
 *          boards.
 * 
 * @param   BmpDecodeHandle *_bmpDecoder
 *          
 * @param   InkplateImageDecodeErrors *_decodeError
 *          
 * @return  bool
 *          true - 
 *          false - 
 */
bool inkplateImageDecodeHelpersBmp(BmpDecodeHandle *_bmpDecoder, InkplateImageDecodeErrors *_decodeError)
{
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

    // If decode process is ok, return true for success.
    return true;
}

/**
 * @brief   
 * 
 * @param   JDEC *_jpgDecoder
 *          
 * @param   size_t (*_inFunc)(JDEC *, uint8_t *, size_t)
 *          
 * @param   int (*_outFunc)(JDEC *, void *, JRECT *)
 *          
 * @param   InkplateImageDecodeErrors *_decodeError
 *          
 * @param   void *_sessionHandler
 *          
 * @return  bool
 *          true - 
 *          false -  
 */
bool inkplateImageDecodeHelpersJpg(JDEC *_jpgDecoder, size_t (*_inFunc)(JDEC *, uint8_t *, size_t), int (*_outFunc)(JDEC *, void *, JRECT *), InkplateImageDecodeErrors *_decodeError, void *_sessionHandler)
{
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

    // If decode process is ok, return true for success.
    return true;
}

/**
 * @brief   
 * 
 * @param   _pngDecoder 
 * @param   _inFunc 
 * @param   _outFunc 
 * @param   _imgW 
 * @param   _imgH 
 * @param   _decodeError 
 * @param   _sessionHandler 
 * @return  true 
 * @return false 
 */
bool inkplateImageDecodeHelpersPng(pngle_t *_pngDecoder, bool (*_inFunc)(pngle_t *_pngle), void (*_outFunc)(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]), int *_imgW, int *_imgH, InkplateImageDecodeErrors *_decodeError, void *_sessionHandler)
{
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
 * @brief 
 * 
 * @param _filename 
 * @param _bytes 
 * @return enum InkplateImageDecodeFormat 
 */
enum InkplateImageDecodeFormat inkplateImageDecodeHelpersDetectImageFormat(char *_filename, void *_bytes)
{
    // First, try to get the image format by it's extension.
    // Firstly try to find the extension itself. It is at the end of the filename.
    // So, first get the lenght of the filename.
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
        printf("File is jpg\r\n");
        return INKPLATE_IMAGE_DECODE_FORMAT_JPG;
    }
    else if (strstr(_extension, ".PNG"))
    {
        printf("File is png\r\n");
        return INKPLATE_IMAGE_DECODE_FORMAT_PNG;
    }

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

    // If this also failed, then this may not be a vaild image format.
    return INKPLATE_IMAGE_DECODE_FORMAT_ERR;
}

/**
 * @brief 
 * 
 * @param _dataPtr 
 * @param _headerSignature 
 * @return true 
 * @return false 
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
 * @brief 
 * 
 * @param _path 
 * @return true 
 * @return false 
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
    
    // More checks can be added, but also can use manual override.
    
    // If nothing has been found, return false.
    return false;
}