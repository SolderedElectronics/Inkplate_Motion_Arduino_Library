// Include Inkplate Motion Library.
#include <InkplateMotion.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

// Include pngle decoder library.
#include "pngle.h"

// Create Inkaplte Motion Class.
Inkplate inkplate;

// Temp buffer for sprintf.
char stringTempBuffer[500];

int _write(int file, char *ptr, int len) {
    // Check if the file descriptor is for stdout or stderr
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        // Write each character to Serial
        for (int i = 0; i < len; i++) {
            if (ptr[i] == '\n') {
                Serial.write('\r');  // Handle newline by adding carriage return
            }
            Serial.write(ptr[i]);  // Send character to Serial
        }
        return len;  // Return the number of characters written
    }
    
    // If the file descriptor is not stdout or stderr, return an error
    errno = EBADF;
    return -1;
}

volatile uint8_t *_rgbBuffer = (uint8_t*)(0xD0600000);

void myPngleOnDraw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
    // Do not allow larger images than display size.
    if ((x > inkplate.width()) || (x < 0) || (y > inkplate.height()) || y < 0) return;

    // Save red, green and blue pixel into the SDRAM buffer.
    _rgbBuffer[x + (1024 * 3 * y)] = rgba[0];
    _rgbBuffer[x + (1024 * 3 * y) + 1] = rgba[1];
    _rgbBuffer[x + (1024 * 3 * y) + 2] = rgba[2];
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
    File file = inkplate.sdFat.open("gradient.png", O_READ);

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

    // Decode it chunk-by-chunk.
    // Allocate new PNG decoder.
    pngle_t *_newPngle = pngle_new();

    // Set the callback for decoder.
    pngle_set_draw_callback(_newPngle, myPngleOnDraw);

    // Let's do decoding!
    printInfoMessage(&inkplate, "Decode start!", 20, false, false, true, false);

    uint32_t total = file.fileSize();
    uint8_t buff[4096];
    uint32_t pnt = 0;
    int remain = 0;

    while (pnt < total)
    {
        uint32_t toread = file.available();
        if (toread > 0)
        {
            int len = file.read(buff, min((uint32_t)2048, toread));
            int fed = pngle_feed(_newPngle, buff, len);
            if (fed < 0)
            {
                sprintf(stringTempBuffer, "PNGLE ERROR: %s\r\n", pngle_error(_newPngle));
                printInfoMessage(&inkplate, stringTempBuffer, 20, false, false, true, false);
                while(1);
            }
            remain = remain + len - fed;
            pnt += len;
        }
    }
    printInfoMessage(&inkplate, "Decode end!", 20, false, false, true, false);

    // Convert it to the grayscale.
    RGBtoGrayscale(&inkplate, 10, 10, _rgbBuffer, pngle_get_width(_newPngle), pngle_get_height(_newPngle));

    // Free allocated memory from the pngle library.
    pngle_destroy(_newPngle);

    inkplate.display();
}

void printInfoMessage(Inkplate *_inkplate, char *_msg, int _startX, bool _leaveOn, bool _continuePrint, bool _newLineBefore, bool _newLineAfter)
{
    if (_newLineBefore) _inkplate->println();
    if (!_continuePrint) inkplate.setCursor(_startX, inkplate.getCursorY());
    inkplate.print(_msg);
    if (_newLineAfter) _inkplate->println();
    inkplate.partialUpdate(_leaveOn);
}

void loop()
{

}

void RGBtoGrayscale(Inkplate *_inkplate, int x, int y, volatile uint8_t *_rgbBuffer, uint16_t _w, uint16_t _h)
{
    for (int _y = 0; _y < _h; _y++)
    {
        for (int _x = 0; _x < _w; _x++)
        {
            uint8_t _r = _rgbBuffer[_x + (1024 * 3 * _y)];
            uint8_t _g = _rgbBuffer[_x + (1024 * 3 * _y) + 1];
            uint8_t _b = _rgbBuffer[_x + (1024 * 3 * _y) + 2];
            uint8_t _pixel = (((2126 * _r) + (7152 * _g) + (722 * _b)) / 10000) >> 4;
            _inkplate->drawPixel(_x + x, _y + y, _pixel);
        }
    }
}

