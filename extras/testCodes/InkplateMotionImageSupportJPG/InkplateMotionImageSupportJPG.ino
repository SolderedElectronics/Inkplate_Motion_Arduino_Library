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

    Serial.printf("read req, size = %d, ptrAddr = 0x%08X\r\n", _nbyte, _buff);
    Serial.flush();

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
int onDraw(JDEC* jd, void* bitmap, JRECT* rect )
{
    Serial.printf("top = %d, botton = %d, left = %d, right = %d\r\n", rect->top, rect->bottom, rect->left, rect->right);
}

void setup()
{
    // Initialize serial for debug.
    Serial.begin(115200);

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
    File file = inkplate.sdFat.open("gradient.jpg", O_READ);

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
        //sprintf(stringTempBuffer, "JPG Decoder prepare failed, rc = %d\r\n", result);
        //printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, false);
        while(1);
    }

    //printInfoMessage(&inkplate, "prepare done, starting decompression", 20, true, false, true, false);
    _sessionId.fbuf = _rgbBuffer;
    _sessionId.wfbuf = jpgDecoder.width;
    result = jd_decomp(&jpgDecoder, onDraw, 0);   /* Start to decompress with 1/1 scaling */
    if (result != JDR_OK)
    {
        //sprintf(stringTempBuffer, "JPG Decoder decpmpression failed, rc = %d\r\n", result);
        //printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, false);
        while(1);
    }

    //printInfoMessage(&inkplate, "decompression done!", 20, true, false, true, false);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void printInfoMessage(Inkplate *_inkplate, char *_msg, int _startX, bool _leaveOn, bool _continuePrint, bool _newLineBefore, bool _newLineAfter)
{
    if (_newLineBefore) _inkplate->println();
    if (!_continuePrint) inkplate.setCursor(_startX, inkplate.getCursorY());
    inkplate.print(_msg);
    if (_newLineAfter) _inkplate->println();
    inkplate.partialUpdate(_leaveOn);
}