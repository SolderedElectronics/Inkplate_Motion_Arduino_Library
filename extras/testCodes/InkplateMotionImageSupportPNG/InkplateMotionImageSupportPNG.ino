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
    uint8_t r = 0xff; // 0 - 255
    uint8_t g = rgba[1]; // 0 - 255
    uint8_t b = rgba[2]; // 0 - 255
    drawIntoFramebuffer(x, y, ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b));
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
    File file = inkplate.sdFat.open("white_green_blue_pattern_image.png", O_READ);

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
            // Calculate the framebuffer array index.
            uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

            uint8_t _r = _rgbBuffer[_fbArrayIndex + 2];
            uint8_t _g = _rgbBuffer[_fbArrayIndex + 1];
            uint8_t _b = _rgbBuffer[_fbArrayIndex];
            uint8_t _pixel = (((2126 * (uint32_t)(_r)) + (7152 * (uint32_t)(_g)) + (722 * (uint32_t)(_b))) / 10000) >> 4;
            _inkplate->drawPixel(_x + x, _y + y, _pixel);
        }
    }
}

void drawIntoFramebuffer(int _x, int _y, uint32_t _color)
{
    if ((_x >= 1024) || (_x < 0) || (_y >= 758) || (_y < 0)) return;

    // Calculate the framebuffer array index.
    uint32_t _fbArrayIndex = (_x + (1024 * _y)) * 3;

    // Write the pixel value.
    _rgbBuffer[_fbArrayIndex + 2] = _color >> 16;
    _rgbBuffer[_fbArrayIndex + 1] = (_color >> 8) & 0xFF;
    _rgbBuffer[_fbArrayIndex] = _color & 0xFF;
}