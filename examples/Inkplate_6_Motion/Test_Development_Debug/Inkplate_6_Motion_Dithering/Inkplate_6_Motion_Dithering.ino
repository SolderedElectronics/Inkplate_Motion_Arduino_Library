#include "InkplateMotion.h"
#include "ditherKernels.h"
#include "newWaveform.h"
#include "dither.h"

Inkplate inkplate;

ImageProcessing imgProcess;

void setup()
{
    // Initialize Serial communication for debugging.
    Serial.begin(115200);
    Serial.println("Hello from Inkplate 6 Motion!");

    // Initialize Inkplate Motion library in 4 bit mode.
    //inkplate.begin(INKPLATE_1BW);
    inkplate.begin(INKPLATE_GL16);

    imgProcess.begin(&inkplate, SCREEN_WIDTH);
    
    // Load the new wavefrom!
    if (!inkplate.loadWaveform(custom4BitWavefrom))
    {
        Serial.println("Wavefrom load failed, halting");

        while (1)
            ;
    }
    else
    {
        Serial.println("New wavefrom loaded!");
    }

    // Clear the screen.
    inkplate.display();

    // Initialze microSD card.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_MICROSD, true);
    
    if (!inkplate.microSDCardInit())
    {
        Serial.println("MicroSD card init failed!\r\n");
        while(1);
    }
    else
    {
        Serial.println("MicroSD card init ok!\r\n");
    }

    // Dispaly the image on the screen.
    unsigned long time1 = micros();
    //inkplate.image.draw("gradient_dithered.png", 0, 0);
    inkplate.image.draw("gradient.png", 0, 0);
    //inkplate.image.draw("rainbow.jpg", 0, 0);
    //inkplate.image.draw("img1.jpg", 0, 0);
    //inkplate.image.draw("pexels-lauma-augstkalne-322733111-28681534.jpg", 0, 0);
    //inkplate.image.draw("pexels-njeromin-29233611.jpg", 0, 0);
    //inkplate.image.draw("pexels-helen1-29238664.jpg", 0, 0);
    //inkplate.image.draw("cat.jpg", 0, 0);
    //inkplate.image.draw("mountain.jpg", 0, 0);
    //inkplate.image.draw("road.jpg", 0, 0);
    unsigned long time2 = micros();
    Serial.printf("DrawImage time: %lu\r\n", time2 - time1);

    // Now do a dither!
    // time1 = millis();
    // if (inkplate.getDisplayMode() == INKPLATE_1BW)
    // {
    //     dither_with_kernel((uint8_t*)(0xD0600000), 1024, 758, FS_KERNEL, FS_KERNEL_SIZE, 2, 1, false);
    // }
    // else
    // {
    //     dither_with_kernel((uint8_t*)(0xD0600000), 1024, 758, SIERRA_KERNEL, SIERRA_KERNEL_SIZE, 2, 4, false);
    // }
    // time2 = millis();
    time1 = micros();
    imgProcess.processImage((uint8_t*)(0xD0600000), 50, 100, 1024, 379, true, false, FS_KERNEL, FS_KERNEL_SIZE, 4);
    time2 = micros();
    Serial.printf("Dithering time: %lu\r\n", time2 - time1);
    inkplate.display();
}

void loop()
{
}

void dither_with_kernel(uint8_t *image, uint16_t width, uint16_t height, const KernelElement *kernel, size_t kernel_size, uint8_t rows_to_buffer, uint8_t output_bit_depth, bool colorInvert)
{
    int16_t *error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t)); // +2 for boundary handling
    if (!error_buffer)
    return;

    int16_t *next_error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t));
    if (!next_error_buffer)
    {
        free(error_buffer);
        return;
    }

    // Allocate the memory for the row buffering.
    uint8_t *row_buffer = (uint8_t *)malloc(rows_to_buffer * width * sizeof(uint8_t));
    if (!row_buffer)
    {
        free(error_buffer);
        free(next_error_buffer);
        return;
    }

    // Load the first `rows_to_buffer` rows into the buffer.
    for (uint8_t i = 0; i < rows_to_buffer && i < height; i++)
    {
        memcpy(&row_buffer[i * width], &image[i * width], width);
    }

    for (uint16_t y = 0; y < height; y++)
    {
        // Select current row.
        uint8_t *current_row = &row_buffer[0];

        for (uint16_t x = 0; x < width; x++)
        {
            // Adjust pixel value with propagated error.
            int16_t old_pixel = current_row[x] + error_buffer[x + 1];

            // Clamp to valid 8-bit range.
            if (old_pixel < 0) old_pixel = 0;
            if (old_pixel > 255) old_pixel = 255;

            uint8_t new_pixel;
            if (output_bit_depth == 1)
            {
                // 1-bit output: threshold to black or white.
                if (old_pixel >= 128)
                {
                    new_pixel = 255;
                }
                else
                {
                    new_pixel = 0;
                }
            }
            else if (output_bit_depth == 4)
            {
                // 4-bit output: quantize to 0-15 (map 0-255 to 0-15).
                new_pixel = (old_pixel * 15 + 127) / 255;
            }
            else
            {
                // Unsupported bit depth.
                new_pixel = 0;
            }

            // Output the pixel depending on the selected bit depth.
            if (output_bit_depth == 1)
            {
                inkplate.drawPixel(x, y, colorInvert?new_pixel:~new_pixel);
            }
            else if (output_bit_depth == 4)
            {
                inkplate.drawPixel(x, y, colorInvert?~new_pixel:new_pixel);
            }

            // Reverse quantization to calculate error.
            int16_t quant_error;
            if (output_bit_depth == 1)
            {
                quant_error = old_pixel - ((new_pixel == 255) ? 255 : 0);
            }
            else if (output_bit_depth == 4)
            {
                quant_error = old_pixel - (new_pixel * 255 / 15);
            } else
            {
                quant_error = 0;
            }

            // Propagate error using the kernel.
            for (size_t i = 0; i < kernel_size; i++)
            {
                int16_t new_x = x + kernel[i].x_offset;
                int16_t new_y = y + kernel[i].y_offset;

                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height)
                {
                    int16_t *target_buffer = (kernel[i].y_offset == 0) ? error_buffer : next_error_buffer;
                    target_buffer[new_x + 1] += (quant_error * kernel[i].weight) / 16;
                }
            }
        }

        // Swap error buffers.
        int16_t *temp = error_buffer;
        error_buffer = next_error_buffer;
        next_error_buffer = temp;

        // Clear the buffer.
        memset(next_error_buffer, 0, (width + 2) * sizeof(int16_t));

        // Load the next row into the buffer, if within bounds
        if (y + rows_to_buffer < height)
        {
             // Shift up!
            memmove(row_buffer, &row_buffer[width], (rows_to_buffer - 1) * width);
            memcpy(&row_buffer[(rows_to_buffer - 1) * width], &image[(y + rows_to_buffer) * width], width);
        }
    }

    // Free allocated memory.
    free(error_buffer);
    free(next_error_buffer);
    free(row_buffer);
}