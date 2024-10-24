/**
 **************************************************
 *
 * @file        helpers.cpp
 * @brief       Source file for the helpers functions.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

// Inclucde header the file.
#include "helpers.h"

Helpers::Helpers()
{
}

/**
 * @brief   Function copy part of the SDRAM and writes it into dedicated DMA buffers.
 *          Transfer is done using MDMA.
 *
 * @param   MDMA_HandleTypeDef *hmdma
 *          STM32 Master DMA pointer to the instance/typedef.
 * @param   uint8_t *_internalBuffer
 *          Internal buffer used for DMA copying. MUST BE INSIDE DTCMRAM!
 * @param   uint32_t _internalBufferSize
 *          Size of the buffer used for reading chunk-by-chunk in bytes.
 *          Recommended size of the buffer is 8kB (8192 bytes).
 * @param   volatile uint8_t *_srcBuffer
 *          Address of the source buffer (SDRAM).
 * @param   volatile uint8_t *_destBuffer
 *          Pointer to the destiantion buffer.
 * @param   uint32_t _size
 *          Total size of the read (in bytes).
 *
 */
void Helpers::copySDRAMBuffers(MDMA_HandleTypeDef *hmdma, uint8_t *_internalBuffer, uint32_t _internalBufferSize,
                               volatile uint8_t *_srcBuffer, volatile uint8_t *_destBuffer, uint32_t _size)
{
    // Calculate how many memory segments there are.
    uint16_t _chunks = _size / _internalBufferSize;

    // Calculate the reminder (last chunk size).
    uint16_t _lastChinkSize = _size % _internalBufferSize;

    // Align chunks to be in 32 bits size.
    _lastChinkSize = MULTIPLE_OF_4(_lastChinkSize);

    for (uint16_t i = 0; i < _chunks; i++)
    {
        // Start DMA transfer from SDRAM to internal STM32 SRAM.
        HAL_MDMA_Start_IT(hmdma, (uint32_t)_srcBuffer, (uint32_t)_destBuffer, _internalBufferSize, 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();

        // Increment the pointers.
        _srcBuffer += _internalBufferSize;
        _destBuffer += _internalBufferSize;
    }

    // Copy the remainder of the last chunk.
    if (_lastChinkSize != 0)
    {
        HAL_MDMA_Start_IT(hmdma, (uint32_t)_srcBuffer, (uint32_t)_destBuffer, _lastChinkSize, 1);
        while (stm32FmcSdramCompleteFlag() == 0)
            ;
        stm32FmcClearSdramCompleteFlag();
    }
}

enum InkplateImageDecodeFormat Helpers::detectImageFormat(char *_filename, void *_bytes)
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
    if (checkHeaders((uint8_t*)_bytes, (uint8_t*)_helpersBmpSignature))
    {
        return INKPLATE_IMAGE_DECODE_FORMAT_BMP;
    }
    else if (checkHeaders((uint8_t*)_bytes, (uint8_t*)_helpersJpgSignature))
    {
        return INKPLATE_IMAGE_DECODE_FORMAT_JPG;
    }
    else if (checkHeaders((uint8_t*)_bytes, (uint8_t*)_helpersPngSignature))
    {
        return INKPLATE_IMAGE_DECODE_FORMAT_PNG;
    }

    // If this also failed, then this may not be a vaild image format.
    return INKPLATE_IMAGE_DECODE_FORMAT_ERR;
}

bool Helpers::checkHeaders(void *_dataPtr, void *_headerSignature)
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

bool Helpers::isWebPath(char *_path)
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