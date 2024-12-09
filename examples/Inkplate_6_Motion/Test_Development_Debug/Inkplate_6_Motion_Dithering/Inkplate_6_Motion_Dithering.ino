#include "InkplateMotion.h"

Inkplate inkplate;

// Dithering Kernel
typedef struct {
    int8_t x_offset;  // X offset from the current pixel
    int8_t y_offset;  // Y offset from the current pixel
    uint8_t weight;   // Weight of the error to propagate (normalized to sum = 16)
} KernelElement;

// Example: Floyd-Steinberg Kernel
const KernelElement FS_KERNEL[] = {
    {1, 0, 7},  // Right
    {-1, 1, 3}, // Bottom-left
    {0, 1, 5},  // Bottom
    {1, 1, 1}   // Bottom-right
};

// Example: Jarvis-Judice-Ninke Kernel
const KernelElement JJN_KERNEL[] = {
    {1, 0, 7}, {2, 0, 5},  // Current row
    {-2, 1, 3}, {-1, 1, 5}, {0, 1, 7}, {1, 1, 5}, {2, 1, 3}, // Next row
    {-2, 2, 1}, {-1, 2, 3}, {0, 2, 5}, {1, 2, 3}, {2, 2, 1}  // Two rows down
};

// Number of elements in the kernel
#define FS_KERNEL_SIZE (sizeof(FS_KERNEL) / sizeof(FS_KERNEL[0]))
#define JJN_KERNEL_SIZE (sizeof(JJN_KERNEL) / sizeof(JJN_KERNEL[0]))


void setup()
{
    // Initialize Serial communication for debugging.
    Serial.begin(115200);
    Serial.println("Hello from Inkplate 6 Motion!");

    // Initialize Inkplate Motion library in 4 bit mode.
    //inkplate.begin(INKPLATE_1BW);
    inkplate.begin(INKPLATE_GL16);
    
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
    inkplate.image.draw("mountain.jpg", 0, 0);
    //inkplate.image.draw("road.jpg", 0, 0);
    unsigned long time2 = micros();
    Serial.printf("DrawImage time: %lu\r\n", time2 - time1);

    // Now do a dither!
    time1 = millis();
    //dither_with_kernel((uint8_t*)(0xD0600000), 1024, 758, FS_KERNEL, FS_KERNEL_SIZE, 2);
    if (inkplate.getDisplayMode() == INKPLATE_1BW)
    {
        dither_with_kernel_advanced((uint8_t*)(0xD0600000), 1024, 758, FS_KERNEL, FS_KERNEL_SIZE, 2, 1, false);
    }
    else
    {
        dither_with_kernel_advanced((uint8_t*)(0xD0600000), 1024, 758, FS_KERNEL, FS_KERNEL_SIZE, 2, 4, false);
    }
    time2 = millis();
    Serial.printf("Dithering time: %lu\r\n", time2 - time1);

        // for (uint16_t y = 0; y < 758; y++)
        // {
        //     for (uint16_t x = 0; x < 1024; x++)
        //     {
        //         inkplate.drawPixel(x, y, ((uint8_t*)(0xD0600000))[y * 1024 + x] >> 4);
        //     }
        // }

    // for (int y = 0; y < 50; y++)
    // {
    //     for (int x = 0; x < 1024; x++)
    //     {
    //         Serial.print(((uint8_t*)(0xD0800000))[y * 1024 + x], DEC);
    //         Serial.print(", ");
    //     }
    //     Serial.println();
    // }

    inkplate.display();
}

void loop() {
  // put your main code here, to run repeatedly:

}

// Floyd-Steinberg Dithering for 8-bit Input and 4-bit Output
void floyd_steinberg_dither(uint8_t *image, uint16_t width, uint16_t height) {
    int16_t *error_buffer = (int16_t *)malloc((width + 2) * sizeof(int16_t)); // +2 for boundary handling
    if (!error_buffer) return; // Check for allocation failure

    int16_t *next_error_buffer = (int16_t *)malloc((width + 2) * sizeof(int16_t));
    if (!next_error_buffer) {
        free(error_buffer);
        return; // Check for allocation failure
    }

    // Initialize buffers to zero
    for (uint16_t i = 0; i < width + 2; i++) {
        error_buffer[i] = 0;
        next_error_buffer[i] = 0;
    }

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            // Adjust pixel value with propagated error
            int16_t old_pixel = image[y * width + x] + error_buffer[x + 1]; // Add propagated error

            // Clamp to valid 8-bit range
            if (old_pixel < 0) old_pixel = 0;
            if (old_pixel > 255) old_pixel = 255;

            // Quantize to nearest 4-bit level
            uint8_t new_pixel = (old_pixel * 15 + 127) / 255; // Map 0–255 to 0–15
            inkplate.drawPixel(x, y, new_pixel);

            // Reverse quantization to calculate error in 8-bit range
            int16_t quant_error = old_pixel - (new_pixel * 255 / 15);

            // Propagate error
            next_error_buffer[x] += (quant_error * 3) / 16;     // Error down
            next_error_buffer[x + 1] += (quant_error * 5) / 16; // Error down-right
            error_buffer[x + 2] += (quant_error * 7) / 16;      // Error right
            next_error_buffer[x + 2] += (quant_error * 1) / 16; // Error down-left
        }

        // Swap error buffers
        int16_t *temp = error_buffer;
        error_buffer = next_error_buffer;
        next_error_buffer = temp;

        // Reset next_error_buffer to zero
        for (uint16_t i = 0; i < width + 2; i++) {
            next_error_buffer[i] = 0;
        }
    }

    free(error_buffer);
    free(next_error_buffer);
}

// Dithering function with generic kernel
void dither_with_kernel(uint8_t *image, uint16_t width, uint16_t height, const KernelElement *kernel, size_t kernel_size, uint8_t rows_to_buffer) {
    int16_t *error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t)); // +2 for boundary handling
    if (!error_buffer) return;

    int16_t *next_error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t));
    if (!next_error_buffer) {
        free(error_buffer);
        return;
    }

    // Allocate buffer for original image rows
    uint8_t *row_buffer = (uint8_t *)malloc(rows_to_buffer * width * sizeof(uint8_t));
    if (!row_buffer) {
        free(error_buffer);
        free(next_error_buffer);
        return;
    }

    // Load the first `rows_to_buffer` rows into the buffer
    for (uint8_t i = 0; i < rows_to_buffer && i < height; i++) {
        memcpy(&row_buffer[i * width], &image[i * width], width);
    }

    for (uint16_t y = 0; y < height; y++) {
        uint8_t *current_row = &row_buffer[0]; // Current row being processed

        for (uint16_t x = 0; x < width; x++) {
            // Adjust pixel value with propagated error
            int16_t old_pixel = current_row[x] + error_buffer[x + 1]; // Add propagated error

            // Clamp to valid 8-bit range
            if (old_pixel < 0) old_pixel = 0;
            if (old_pixel > 255) old_pixel = 255;

            // Quantize to nearest 4-bit level
            uint8_t new_pixel = (old_pixel * 15 + 127) / 255; // Map 0–255 to 0–15
            inkplate.drawPixel(x, y, new_pixel);

            // Reverse quantization to calculate error in 8-bit range
            int16_t quant_error = old_pixel - (new_pixel * 255 / 15);

            // Propagate error using the kernel
            for (size_t i = 0; i < kernel_size; i++) {
                int16_t new_x = x + kernel[i].x_offset;
                int16_t new_y = y + kernel[i].y_offset;

                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) {
                    int16_t *target_buffer = (kernel[i].y_offset == 0) ? error_buffer : next_error_buffer;
                    target_buffer[new_x + 1] += (quant_error * kernel[i].weight) / 16;
                }
            }
        }

        // Swap error buffers
        int16_t *temp = error_buffer;
        error_buffer = next_error_buffer;
        next_error_buffer = temp;

        // Reset next_error_buffer to zero
        memset(next_error_buffer, 0, (width + 2) * sizeof(int16_t));

        // Load the next row into the buffer, if within bounds
        if (y + rows_to_buffer < height) {
            memmove(row_buffer, &row_buffer[width], (rows_to_buffer - 1) * width); // Shift up
            memcpy(&row_buffer[(rows_to_buffer - 1) * width], &image[(y + rows_to_buffer) * width], width);
        }
    }

    free(error_buffer);
    free(next_error_buffer);
    free(row_buffer);
}

void dither_to_1bit(uint8_t *image, uint16_t width, uint16_t height, const KernelElement *kernel, size_t kernel_size) {
    int16_t *error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t)); // +2 for boundary handling
    if (!error_buffer) return;

    int16_t *next_error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t));
    if (!next_error_buffer) {
        free(error_buffer);
        return;
    }

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            // Add propagated error to the current pixel
            int16_t old_pixel = image[y * width + x] + error_buffer[x + 1];

            // Clamp to valid range
            if (old_pixel < 0) old_pixel = 0;
            if (old_pixel > 255) old_pixel = 255;

            // Threshold the pixel to 0 (white) or 1 (black)
            uint8_t new_pixel = (old_pixel >= 128) ? 0 : 1;

            // Output the pixel
            inkplate.drawPixel(x, y, new_pixel);

            // Calculate quantization error
            int16_t quant_error = old_pixel - (new_pixel == 0 ? 255 : 0);

            // Propagate the error using the kernel
            for (size_t i = 0; i < kernel_size; i++) {
                int16_t new_x = x + kernel[i].x_offset;
                int16_t new_y = y + kernel[i].y_offset;

                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) {
                    int16_t *target_buffer = (kernel[i].y_offset == 0) ? error_buffer : next_error_buffer;
                    target_buffer[new_x + 1] += (quant_error * kernel[i].weight) / 16;
                }
            }
        }

        // Swap error buffers
        int16_t *temp = error_buffer;
        error_buffer = next_error_buffer;
        next_error_buffer = temp;

        // Reset next_error_buffer to zero
        memset(next_error_buffer, 0, (width + 2) * sizeof(int16_t));
    }

    free(error_buffer);
    free(next_error_buffer);
}

void dither_with_kernel_advanced(
    uint8_t *image,
    uint16_t width,
    uint16_t height,
    const KernelElement *kernel,
    size_t kernel_size,
    uint8_t rows_to_buffer,
    uint8_t output_bit_depth, // 1 for 1-bit, 4 for 4-bit
    bool colorInvert
) {
    int16_t *error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t)); // +2 for boundary handling
    if (!error_buffer) return;

    int16_t *next_error_buffer = (int16_t *)calloc((width + 2), sizeof(int16_t));
    if (!next_error_buffer) {
        free(error_buffer);
        return;
    }

    uint8_t *row_buffer = (uint8_t *)malloc(rows_to_buffer * width * sizeof(uint8_t));
    if (!row_buffer) {
        free(error_buffer);
        free(next_error_buffer);
        return;
    }

    // Load the first `rows_to_buffer` rows into the buffer
    for (uint8_t i = 0; i < rows_to_buffer && i < height; i++) {
        memcpy(&row_buffer[i * width], &image[i * width], width);
    }

    for (uint16_t y = 0; y < height; y++) {
        uint8_t *current_row = &row_buffer[0];

        for (uint16_t x = 0; x < width; x++) {
            // Adjust pixel value with propagated error
            int16_t old_pixel = current_row[x] + error_buffer[x + 1];

            // Clamp to valid 8-bit range
            if (old_pixel < 0) old_pixel = 0;
            if (old_pixel > 255) old_pixel = 255;

            uint8_t new_pixel;
            if (output_bit_depth == 1) {
                // 1-bit output: threshold to black or white
                if (old_pixel >= 128) { // Black
                    new_pixel = 255;
                } else { // White
                    new_pixel = 0;
                }
            } else if (output_bit_depth == 4) {
                // 4-bit output: quantize to 0-15
                new_pixel = (old_pixel * 15 + 127) / 255; // Map 0–255 to 0–15
            } else {
                // Unsupported bit depth
                new_pixel = 0;
            }

            // Output the pixel
            if (output_bit_depth == 1)
            {
                inkplate.drawPixel(x, y, colorInvert?new_pixel:~new_pixel);
            }
            else if (output_bit_depth == 4)
            {
                inkplate.drawPixel(x, y, colorInvert?~new_pixel:new_pixel);
            }

            // Reverse quantization to calculate error
            int16_t quant_error;
            if (output_bit_depth == 1) {
                quant_error = old_pixel - ((new_pixel == 255) ? 255 : 0);
            } else if (output_bit_depth == 4) {
                quant_error = old_pixel - (new_pixel * 255 / 15);
            } else {
                quant_error = 0;
            }

            // Propagate error using the kernel
            for (size_t i = 0; i < kernel_size; i++) {
                int16_t new_x = x + kernel[i].x_offset;
                int16_t new_y = y + kernel[i].y_offset;

                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) {
                    int16_t *target_buffer = (kernel[i].y_offset == 0) ? error_buffer : next_error_buffer;
                    target_buffer[new_x + 1] += (quant_error * kernel[i].weight) / 16;
                }
            }
        }

        // Swap error buffers
        int16_t *temp = error_buffer;
        error_buffer = next_error_buffer;
        next_error_buffer = temp;

        // Reset next_error_buffer to zero
        memset(next_error_buffer, 0, (width + 2) * sizeof(int16_t));

        // Load the next row into the buffer, if within bounds
        if (y + rows_to_buffer < height) {
            memmove(row_buffer, &row_buffer[width], (rows_to_buffer - 1) * width); // Shift up
            memcpy(&row_buffer[(rows_to_buffer - 1) * width], &image[(y + rows_to_buffer) * width], width);
        }
    }

    free(error_buffer);
    free(next_error_buffer);
    free(row_buffer);
}