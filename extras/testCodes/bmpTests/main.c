#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include "defines.h"
#include "helpers.h"

uint8_t _framebuffer[1024*758*3];

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
    file = fopen(argv[1], "r");

    if (!file)
    {
        printErrorMessage("File open failed!");
        return 0;
    }

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

    processBmp(file, &bmpFileHeader, _framebuffer, sizeof(_framebuffer), 1024);

    printRawFbData(_framebuffer, 1024, 20, &bmpFileHeader);

    return 0;
}