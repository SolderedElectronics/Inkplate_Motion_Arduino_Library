#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include "defines.h"
#include "helpers.h"

#define FRAME_BUFFER_W      1024
#define FRAME_BUFFER_H      758
#define FRAME_BUFFER_BPP    3

uint8_t _framebuffer[FRAME_BUFFER_W * FRAME_BUFFER_H * FRAME_BUFFER_BPP];

int main(int argc, char *argv[])
{
    // Check if the image filename is provided.
    if ((argc < 2) || (argc > 2))
    {
        setConsoleColor(CONSOLE_COLOR_YELLOW);
        printf("Provide image filename with no spaces!");
    }

    // Try to open the provied file.
    FILE *file;
    file = fopen(argv[1], "rb");

    if (!file)
    {
        printErrorMessage("File open failed!");
        return 0;
    }

    // Initialize BMP decoder.
    initBmpDecoder((uint8_t*)_framebuffer, FRAME_BUFFER_W, FRAME_BUFFER_H);

    // Check if the file is vaild BMP file.
    if (!vaildFile(file))
    {
        printErrorMessage("File validation error!");
        printErrorMessage("Error code %d", errCode());
        return 0;
    }

    // Process BMP header.
    bmpHeader bmpFileHeader;
    if (!processHeader(file, &bmpFileHeader))
    {
        printErrorMessage("Header Process failed!");
        printErrorMessage("Error code %d", errCode());
        return 0;
    }

    // Check if this is BMP file at all.
    if (!vaildBMP(&bmpFileHeader))
    {
        printErrorMessage("Not a vaild BMP file!");
        printErrorMessage("Error code %d", errCode());
        return 0;
    }

    printFileInfo(&bmpFileHeader);

    processBmp(file, &bmpFileHeader);

    //printRawFbData(_framebuffer, 1024, 40, &bmpFileHeader);

    // Try to dump framebuffer.
    if (!dumpRawFbData(758, &bmpFileHeader))
    {
        printErrorMessage("Framebuffer dump failed!");
    }

    return 0;
}