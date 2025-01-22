/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Images_From_SD.ino
 * @brief       Draw a list of images from the MicroSD card
 *
 *              Use the official SD card formatter: https://www.sdcard.org/downloads/formatter/
 *              Format the card to FAT32, copy the images from this sketch folder on the SD card
 *              The supported formats are .bmp, .png and .jpg (without progressive encoding)
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Incldue Inkplate Motion library
#include "InkplateMotion.h"

// Create Inkplate Motion object
Inkplate inkplate;

void setup()
{
    // Initialize Inkplate Motion Library. Use 4 bit mode
    inkplate.begin(INKPLATE_GL16);

    // Try to initialize the card
    if (!inkplate.microSDCardInit())
    {
        // There was an error!
        // Set text options so the user can see the error message
        inkplate.setTextColor(0, 15);
        inkplate.setTextSize(3);
        inkplate.display();
        // Show message on the screen
        inkplate.println("microSD card init failed!");
        inkplate.partialUpdate();

        // Stop the code!
        while (1)
            ;
    }
    // MicroSD init was successful

    /*
    Let's draw image1.png, with dithering
    First parameters is the image path
    Second and third parameters are x, y coordinates where to print the image (upper left corner)
    Fourth parameter is color invert (true, false)
    Fifth is dither (1 - enabled, 0 - disabled)
    Fourth and fifth are kernel selection, you can use:
        FS_KERNEL: Floyd-Steinberg
        STUCKI_KERNEL: Stucki
        SIERRA_KERNEL: Sierra
        SIERRA_LITE_KERNEL: Sierra Lite
        ATKINSON_KERNEL: Atkinson
        BURKES_KERNEL: Burke
    The fifth parameter, depending on the Kernel just has the suffix '_SIZE', eg. FS_KERNEL_SIZE
    */

    if (!inkplate.image.draw("image1.png", 0, 0, false, 1, FS_KERNEL, FS_KERNEL_SIZE))
    {
        // Show error on the screen if decode failed
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        // Otherwise, refresh the screen with a partial update
        inkplate.partialUpdate();
    }

    delay(5000); // Wait 5 seconds so that the image can be viewed

    /*
    Let's draw image2.jpg, without dithering
    Use 0, NULL, 0 for the last parameters if dithering isn't used
    */

    if (!inkplate.image.draw("image2.jpg", 0, 0, false, 0, NULL, 0))
    {
        // Show error on the screen if decode failed
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        inkplate.partialUpdate();
    }

    delay(5000); // Wait 5 seconds so that the image can be viewed

    /*
    Let's draw image3.bmp, use Sierra Lite dithering
    */

    if (!inkplate.image.draw("image3.bmp", 0, 0, false, 1, SIERRA_LITE_KERNEL, SIERRA_LITE_KERNEL_SIZE))
    {
        // Show error on the screen if decode failed
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.display();
    }
    else
    {
        // Otherwise, refresh the screen
        inkplate.display();
    }

    delay(5000); // Wait 5 seconds so that the image can be viewed
}

void loop()
{
}
