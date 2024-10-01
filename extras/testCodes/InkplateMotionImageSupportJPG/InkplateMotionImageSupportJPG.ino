// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Include JPG decoder.
#include "tjpgd.h"

// Create Inkplate Motion object.
Inkplate inkplate;

// Temp buffer for sprintf.
char stringTempBuffer[500];

volatile uint8_t *_rgbBuffer = (uint8_t*)(0xD0600000);

/* Session identifier for input/output functions (Name, members and usage are as user defined) */
typedef struct {
    File *fp;               /* Input stream */
    volatile uint8_t *fbuf;          /* Pointer to the frame buffer */
    unsigned int wfbuf;     /* Width of the frame buffer [pix] */
} IODEV;

/**
 * @brief   User defined input function.
 * 
 * @param   JDEC* jd
 *          Decompression object.
 * @param   uint8_t* buff
 *          Pointer to the read buffer (null to remove data)
 * @param   size_t nbyte
 *          Number of bytes to read/remove
 * 
 * @return  Returns number of bytes read (zero on error)
 * 
 */
size_t inputDataFeeder(JDEC* _jd, uint8_t* _buff, size_t _nbyte)
{
    // Session identifier (5th argument of jd_prepare function).
    IODEV *_dev = (IODEV*)_jd->device;

    // Return value.
    size_t _retValue = 0;

    //Serial.printf("read req, size = %d, ptrAddr = 0x%08X\r\n", _nbyte, _buff);
    //Serial.flush();

    if (_buff)
    {
        _retValue = _dev->fp->readBytes(_buff, _nbyte);
    }
    else
    {
        _retValue = _dev->fp->seekCur(_nbyte)?_nbyte:0;
    }

    // Return the result.
    return _retValue;
}

/**
 * @brief   User defined output/callback function.
 * 
 * @param   JDEC* jd
 *          Decompression object.
 * @param   void* bitmap
 *          Bitmap data to be output.
 * @param   JRECT* rect
 *          Rectangle region of output image.
 * 
 * @return  Returns number of bytes read (zero on error).
 */
int onDraw(JDEC* _jd, void* _bitmap, JRECT* _rect)
{
    //Serial.printf("[Output feed] R=%d, L=%d, T=%d, B=%d, W=%d H=%d\r\n", _rect->right, _rect->left, _rect->top, _rect->bottom, abs(_rect->left - _rect->right), abs(_rect->top - _rect->bottom));
    //Serial.flush();

    // Session identifier (5th argument of jd_prepare function).
    IODEV *_dev = (IODEV*)_jd->device;

    // Calculate the width and height.
    int _w = abs(_rect->right - _rect->left) + 1;
    int _h = abs(_rect->bottom - _rect->top) + 1;

    // Get the start positions.
    int _x0 = _rect->left;
    int _y0 = _rect->top;

    // Use 8 bits for the bitmap representation.
    uint8_t *_decodedData = (uint8_t*)(_bitmap);

    // Write the pixels into the framebuffer!
    // DMA2D could be used here?

    int _size = _w * _h * 3;

    // Serial.printf("W=%d, H=%d, _size=%d\r\n", _w, _h, _size);

    // for (int i = 0; i < _size; i++)
    // {
    //     Serial.printf("%c%3d",(i % 3) == 0?'|':' ', _decodedData[i]);
    // }
    // Serial.println();

    for (int _y = 0; _y < _h; _y++)
    {
        for (int _x = 0; _x < _w; _x++)
        {
            _rgbBuffer[(_x + _x0 + (1024 * (_y + _y0))) * 3] =  _decodedData[(_x + (_w * _y)) * 3];
            _rgbBuffer[((_x + _x0 + 1) + (1024 * (_y + _y0))) * 3] = _decodedData[((_x + 1) + (_w * _y)) * 3];
            _rgbBuffer[((_x + _x0 + 2) + (1024 * (_y + _y0))) * 3] = _decodedData[((_x + 2) + (_w * _y)) * 3];
        }
    }

    // Return 1 for success.
    return 1;
}

void setup()
{
    // Initialize serial for debug.
    Serial.begin(115200);
    Serial.println("Code started!");

    // Include Inkplate Motion Library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Set text settings for the graphical engine.
    inkplate.setTextSize(2);
    inkplate.setCursor(20, 20);
    inkplate.setTextColor(0, 15);

    // Clear everything from the screen (just in case).
    inkplate.display();

    // Clear image frame buffer.
    memset((uint8_t*)_rgbBuffer, 15, 1024 * 758 * 3);

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
    File file = inkplate.sdFat.open("cat.jpg", O_READ);

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

    printInfoMessage(&inkplate, "Decode start!", 20, false, false, true, false);

    // Working buffer size.
    size_t _workingBufferSize = 3500;
    void *_workingBuffer;

    // Allocate the memory for the buffer.
    _workingBuffer = (void*)malloc(_workingBufferSize);
    if (_workingBuffer == NULL)
    {
        printInfoMessage(&inkplate, "Allocation failed", 20, false, false, true, false);
        while (1);
    }

    // Prepare JPG decoder.
    JDEC jpgDecoder;
    IODEV _sessionId;
    _sessionId.fp = &file;
    JRESULT result = jd_prepare(&jpgDecoder, inputDataFeeder, _workingBuffer, _workingBufferSize, &_sessionId);

    // Check the result. Notify and halt if failed.
    if (result != JDR_OK)
    {
        sprintf(stringTempBuffer, "JPG Decoder prepare failed, rc = %d\r\n", result);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, false);
        while(1);
    }

    printInfoMessage(&inkplate, "Prepare done, starting decompression", 20, true, false, true, false);
    _sessionId.fbuf = _rgbBuffer;
    _sessionId.wfbuf = jpgDecoder.width;
    result = jd_decomp(&jpgDecoder, onDraw, 0);   /* Start to decompress with 1/1 scaling */
    if (result != JDR_OK)
    {
        sprintf(stringTempBuffer, "JPG Decoder decpmpression failed, rc = %d\r\n", result);
        printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, false);
        while(1);
    }

    printInfoMessage(&inkplate, "Decompression done!", 20, true, false, true, false);

    delay(1000);

    // Convert it to the grayscale.
    RGBtoGrayscale(&inkplate, 10, 10, _rgbBuffer, jpgDecoder.width, jpgDecoder.height);

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

void RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h)
{
    for (int _y = 0; _y < _h; _y++)
    {
        for (int _x = 0; _x < _w; _x++)
        {
            uint8_t _r = _rgbBuffer[(_x + (_w * _y)) * 3];
            uint8_t _g = _rgbBuffer[((_x + 1) + (_w * _y)) * 3];
            uint8_t _b = _rgbBuffer[((_x + 2) + (_w * _y)) * 3];
            uint8_t _pixel = (((2126 * _r) + (7152 * _g) + (722 * _b)) / 10000) >> 4;
            _inkplate->drawPixel(_x + x, _y + y, _pixel);
        }
    }
}