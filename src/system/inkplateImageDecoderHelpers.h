/**
 **************************************************
 *
 * @file        inkplateImageDecoderHelpers.h
 * @brief       Header file for the image decoder helper
 *              functions. These functions are common for all boards.
 *              There are also typedefs used by the ImageDecoder which
 *              is specific for each board.
 *
 *
 * @copyright   GNU General Public License v3.0
 * @authors     Borna Biro for soldered.com
 ***************************************************/

#ifndef __INKPLATE_MOTION_IMAGE_DECODE_PROCESS_H__
#define __INKPLATE_MOTION_IMAGE_DECODE_PROCESS_H__

// Include Arduino Header file to include all needed C/C++ standard libraries.
#include "Arduino.h"

// Include all needed image decoders.
#include "../libs/TJpgDec/tjpgd.h"
#include "../libs/bmpDecode/bmpDecode.h"
#include "../libs/pngle/pngle.h"

// Used by the image decoder for detecting different image formats.
enum InkplateImageDecodeFormat
{
    INKPLATE_IMAGE_DECODE_FORMAT_ERR = 0,
    INKPLATE_IMAGE_DECODE_FORMAT_AUTO,
    INKPLATE_IMAGE_DECODE_FORMAT_BMP,
    INKPLATE_IMAGE_DECODE_FORMAT_JPG,
    INKPLATE_IMAGE_DECODE_FORMAT_PNG,
};

// Used for selecting (manual override) of the paths of the image.
enum InkplateImagePathType
{
    INKPLATE_IMAGE_DECODE_PATH_AUTO = 0,
    INKPLATE_IMAGE_DECODE_PATH_WEB,
    INKPLATE_IMAGE_DECODE_PATH_SD,
};

// List of possible errors while decoding the image. Can be added if needed.
// NOTE: do not add error from each decoder here since the have their own methods.
enum InkplateImageDecodeErrors
{
    INKPLATE_IMAGE_DECODE_NO_ERR = 0,
    INKPLATE_IMAGE_DECODE_ERR_BAD_PARAM,
    INKPLATE_IMAGE_DECODE_ERR_UNKNOWN_FORMAT,
    INKPLATE_IMAGE_DECODE_ERR_FILE_OPEN_FAIL,
    INKPLATE_IMAGE_DECODE_ERR_NO_MEMORY,
    INKPLATE_IMAGE_DECODE_ERR_BMP_DECODER_FAULT,
    INKPLATE_IMAGE_DECODE_ERR_JPG_DECODER_FAULT,
    INKPLATE_IMAGE_DECODE_ERR_PNG_DECODER_FAULT,
    INKPLATE_IMAGE_DECODE_ERR_BMP_HARD_FAULT,
};

// First element = number of bytes in format signature. It's a hack, I know...
static const uint8_t _helpersBmpSignature[3] = {2, 0x42, 0x4D};
static const uint8_t _helpersJpgSignature[4] = {3, 0xFF, 0xD8, 0xFF};
static const uint8_t _helpersPngSignature[9] = {8, 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

bool inkplateImageDecodeHelpersBmp(BmpDecodeHandle *_bmpDecoder, InkplateImageDecodeErrors *_decodeError);
bool inkplateImageDecodeHelpersJpg(JDEC *_jpgDecoder, size_t (*_inFunc)(JDEC *, uint8_t *, size_t),
                                   int (*_outFunc)(JDEC *, void *, JRECT *), InkplateImageDecodeErrors *_decodeError,
                                   void *_sessionHandler);
bool inkplateImageDecodeHelpersPng(pngle_t *_pngDecoder, bool (*_inFunc)(pngle_t *_pngle),
                                   void (*_outFunc)(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                                    uint8_t rgba[4]),
                                   int *_imgW, int *_imgH, InkplateImageDecodeErrors *_decodeError,
                                   void *_sessionHandler);
enum InkplateImageDecodeFormat inkplateImageDecodeHelpersDetectImageFormat(char *_filename, void *_bytes);
bool inkplateImageDecodeHelpersCheckHeaders(void *_dataPtr, void *_headerSignature);
bool inkplateImageDecodeHelpersIsWebPath(char *_path);
#endif