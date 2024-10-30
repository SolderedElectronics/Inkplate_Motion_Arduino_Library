// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Include BMP Decoder librarry.
#include "bmpDecode.h"

// Create Inkaplte Motion Class.
Inkplate inkplate;

// Temp buffer for sprintf.
char stringTempBuffer[500];

// Decode framebuffer in SDRAM.
volatile uint8_t *_rgbBuffer = (uint8_t*)(0xD0600000);

// Session typedef handler.
typedef struct
{
    SdFat *sdFat;
    File *file;
}bmpDecoderSessionHandler;

size_t readBytesFromSd(bmpDecode_t *_bmpDecodeHandler, void *_buffer, uint64_t _n)
{
    // Get the session typedef from the bmpDecoder handler.
    bmpDecoderSessionHandler *_session = (bmpDecoderSessionHandler*)_bmpDecodeHandler->sessionHandler;

    // Try to read requested number of bytes. If buffer is null, use file seek.ß
    // Return value.
    size_t _retValue = 0;

    if (_buffer)
    {
        _retValue = _session->file->readBytes((uint8_t*)_buffer, _n);
    }
    else
    {
        _retValue = _session->file->seekSet(_n)?_n:0;
    }

    // Return the result.
    return _retValue;
}

void setup()
{
    // Initialize serial for debug.
    Serial.begin(115200);

    // Print welcome debug message.
    Serial.println("Hello from Inkplate!");

    // Include Inkplate Motion Library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Set text settings for the graphical engine.
    inkplate.setTextSize(2);
    inkplate.setCursor(20, 20);
    inkplate.setTextColor(0, 15);

    // Clear everything from the screen (just in case).
    inkplate.display();

    // Initialize microSD Card.
    printInfoMessage(&inkplate, "microSD card initialization...", 20, true, false, false, false);
    if (!inkplate.microSDCardInit())
    {
        printInfoMessage(&inkplate, "failed! Code halt!", 20, false, true, false, false);
        while(1);
    }
    else
    {
        printInfoMessage(&inkplate, "ok!", 20, false, true, false, true);
    }

    // First, open the file.
    File file = inkplate.sdFat.open("img04.bmp", O_READ);

    // Check for the file open success.
    printInfoMessage(&inkplate, "File open ", 20, true, false, false, false);
    if (!file)
    {
        printInfoMessage(&inkplate, "failed! Program halt!", 20, false, true, false, false);
        while(1);
    }
    else
    {
        printInfoMessage(&inkplate, "succ!", 20, false, true, false, true);
    }

    // Get the file size.
    uint32_t fileSize = file.size();

    // Print out file size.
    sprintf(stringTempBuffer, "Filesize: %ld", fileSize);
    printInfoMessage(&inkplate, stringTempBuffer, 20, true, false, false, false);

    // Let's initialize BMP decoder.
    bmpDecode_t bmpDec;
    bmpDecoderSessionHandler sessionData;
    bmpDec.inputFeed = &readBytesFromSd;
    bmpDec.errorCode = BMP_DECODE_NO_ERROR;
    sessionData.file = &file;
    bmpDec.output = &drawIntoFramebuffer;
    bmpDec.sessionHandler = &sessionData;


    Serial.println("Preparing to read the header!");
    Serial.flush();

    // Try to read if the file is vaild.
    if (!bmpDecodeVaildFile(&bmpDec))
    {
        sprintf(stringTempBuffer, "Erorr file reading header, error code: %d\r\n", bmpDec.errorCode);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, true);

        // Stop the code!
        while(1);
    }

    printInfoMessage(&inkplate, "File is a bitmap", 20, true, false, true, false);

    // Try to decode a BMP header.
    if (!bmpDecodeProcessHeader(&bmpDec))
    {
        sprintf(stringTempBuffer, "Erorr header processing, error code: %d\r\n", bmpDec.errorCode);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, true);

        // Stop the code!
        while(1);
    }

    printFileInfo(&bmpDec);

    // Check if the BMP is a valid BMP for the decoder.
    if (!bmpDecodeVaildBMP(&bmpDec))
    {
        sprintf(stringTempBuffer, "Not a valid BMP for the decoder, error code: %d\r\n", bmpDec.errorCode);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, true);

        // Stop the code!
        while(1); 
    }

    printInfoMessage(&inkplate, "Ready for the BMP pixel processing!", 20, true, false, true, false);

    if (!bmpDecodeProcessBmp(&bmpDec))
    {
        sprintf(stringTempBuffer, "Pixel processing failed, error code: %d\r\n", bmpDec.errorCode);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, true);

        // Stop the code!
        while(1); 
    }

    // Check for bounds one more time.
    int imageW = min(1024, (int)(bmpDec.header.infoHeader.width));
    int imageH = min(758, (int)(bmpDec.header.infoHeader.height));

    // Convert it to the grayscale.
    RGBtoGrayscale(&inkplate, 10, 10, _rgbBuffer, imageW, imageH);

    // Refresh the screen.
    inkplate.display();
}

void loop()
{

}

void printInfoMessage(Inkplate *_inkplate, char *_msg, int _startX, bool _leaveOn, bool _continuePrint, bool _newLineBefore, bool _newLineAfter)
{
    if (_newLineBefore) _inkplate->println();
    if (!_continuePrint) inkplate.setCursor(_startX, inkplate.getCursorY());
    inkplate.print(_msg);
    if (_newLineAfter) _inkplate->println();
    inkplate.partialUpdate(_leaveOn);
}

void printFileInfo(bmpDecode_t *_bmpDec)
{
    Serial.printf("Header Signature: 0x%04X\r\n", _bmpDec->header.header.signature);
    Serial.printf("File size: %lu\r\n", _bmpDec->header.header.fileSize);
    Serial.printf("Data offset: %lu\r\n", _bmpDec->header.header.dataOffset);
    Serial.printf("Info header size: %lu\r\n", _bmpDec->header.infoHeader.headerSize);
    Serial.printf("Image width: %lu\r\n", _bmpDec->header.infoHeader.width);
    Serial.printf("Image height: %lu\r\n", _bmpDec->header.infoHeader.height);
    Serial.printf("Number of planes: %u\r\n", _bmpDec->header.infoHeader.planes);
    Serial.printf("Depth: %u\r\n", _bmpDec->header.infoHeader.bitCount);
    Serial.printf("Compression: %lu\r\n", _bmpDec->header.infoHeader.compression);
    Serial.printf("Compressed Image Size: %lu\r\n", _bmpDec->header.infoHeader.imageSize);
    Serial.printf("X Pixels Per Meter: %lu\r\n", _bmpDec->header.infoHeader.xPixelsPerM);
    Serial.printf("Y Pixels Per Meter: %lu\r\n", _bmpDec->header.infoHeader.yPixelsPerM);
    Serial.printf("Colors Used: %lu\r\n", _bmpDec->header.infoHeader.colorsUsed);
    Serial.printf("Colors Important: %lu\r\n", _bmpDec->header.infoHeader.colorsImportant);
    Serial.printf("Custom color palette: %s\r\n", _bmpDec->header.customPalette?"Yes":"No");
    if (_bmpDec->header.customPalette)
    {
        uint16_t _noOfColors = 1 << _bmpDec->header.infoHeader.bitCount;
        for (int i = 0; i < _noOfColors; i++)
        {
            Serial.printf("Color %d - R:%3d G:%3d B:%3d\r\n", i, _bmpDec->header.colorTable[i].red, _bmpDec->header.colorTable[i].green, _bmpDec->header.colorTable[i].blue);
        }
    }
}

void RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h)
{
    for (int _y = 0; _y < _h; _y++)
    {
        for (int _x = 0; _x < _w; _x++)
        {
            // Calculate the framebuffer array index.
            uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

            // get the individual RGB colors.
            uint8_t _r = _rgbBuffer[_fbArrayIndex + 2];
            uint8_t _g = _rgbBuffer[_fbArrayIndex + 1];
            uint8_t _b = _rgbBuffer[_fbArrayIndex];

            // Convert them into grayscale.
            uint8_t _pixel = ((77 * _r) + (150 * _g) + (29 * _b)) >> 12;

            // Write into epaper framebuffer.
            _inkplate->drawPixel(_x + x, _y + y, _pixel);
        }
    }
}

// // This should be an external function, used with callback within the decoder.
void drawIntoFramebuffer(bmpDecode_t *_bmpDecodeHandle, int16_t _x, int16_t _y, uint32_t _color)
{
    // Check for bounds.
    if ((_x >= 1024) || (_x < 0) || (_y >= 758) || (_y < 0))
    {
        // Add error if the image is out of bounds.
        _bmpDecodeHandle->errorCode = BMP_DECODE_ERR_IMAGE_OUT_OF_BORDER;

        // Go out!
        return;
    }

    // Check for the null-pointer on the framebuffer.

    // Calculate the framebuffer array index
    uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

    // Write the pixel value.
    _rgbBuffer[_fbArrayIndex + 2] = _color >> 16;
    _rgbBuffer[_fbArrayIndex + 1] = (_color >> 8) & 0xFF;
    _rgbBuffer[_fbArrayIndex] = _color & 0xFF;
}