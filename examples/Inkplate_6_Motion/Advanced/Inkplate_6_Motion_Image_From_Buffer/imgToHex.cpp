#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <stdint.h>

// Change this to match the filename.
#define FILENAME_INPUT "img1.jpg"

// Change these to match your output filename and array.
#define FILENAME_OUTPUT "img1.h"
#define ARRAY_NAME "image1"

int main()
{
    // Pointers to the files.
    FILE *fileInput;
    FILE *fileOutput;

    // Try to open file in binary mode.
    fileInput = fopen(FILENAME_INPUT, "rb");

    if (fileInput == NULL)
    {
        printf("Input file open failed.\r\n");
        system("pause");
    }

    // Try to create the file for the header file.
    fileOutput = fopen(FILENAME_OUTPUT, "w");
    if (fileOutput == NULL)
    {
        printf("Output file create failed.\r\n");
        system("pause");
    }

    // Print a message.
    printf("File opened and created successfully.\n");

    // Create the header.
    fprintf(fileOutput, "const uint8_t %s[] PROGMEM =\n{", ARRAY_NAME);

    // Get the file fize.
    fseek(fileInput, 0, SEEK_END);
    size_t fileSize = ftell(fileInput);
    printf("File size: %lu bytes\r\n", fileSize);

    // Go trough the file byte-by-byte.
    for (int i = 0; i < fileSize; i++)
    {
        // Add every 100 bytes a new line.
        if ((i % 100) == 0) fprintf(fileOutput, "\n    ");

        // Read the byte and advance to the new position.
        uint8_t data = 0;
        fseek(fileInput, i, SEEK_SET);
        fread(&data, 1, 1, fileInput);

        // Sve the HEX data into output file.
        fprintf(fileOutput, "0x%02X, ", data);
    }

    // Close the header in the output file.
    fprintf(fileOutput, "\n};");

    // Close the files.
    fclose(fileInput);
    fclose(fileOutput);

    // Print a message.
    printf("Done!\n");

    // Wait for the user input.
    system("pause");

    // End the program.
    return 0;
}