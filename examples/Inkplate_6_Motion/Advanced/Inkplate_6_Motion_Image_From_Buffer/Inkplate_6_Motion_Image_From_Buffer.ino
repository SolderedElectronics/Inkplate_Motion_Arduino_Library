// Include Inkplate Motion Library.
#include "InkplateMotion.h"

// Include one of the images. To get this image raw data, use imageToHex.py
// script. This script will convert BMP, JPG or PNG file into .h byte array.
#include "parrotSolderedBmp.h"
//#include "parrotSolderedJpg.h"
//#include "parrotSolderedPng.h"

// Create Inkplate Motion object.,
Inkplate inkplate;

void setup()
{
    // Initialize serial communication.
    Serial.begin(115200);

    // Send message on serial so we know that Inkplate is alive.
    Serial.println("Hello from Inkplate 6 Motion!");
    Serial.flush();

    // Initialize Inkplate Motion Library. Use 4 bit color mode.
    inkplate.begin(INKPLATE_GL16);
    inkplate.setTextColor(0, 15);
    inkplate.display();

    // Show message on the screen.
    inkplate.println("loading image from the SRAM, please wait...");
    inkplate.partialUpdate();

    // Use one of the included images.
    // First parameter is byte array of the image, second is size of the array, third and forth are X and Y, fifth
    // is the color inversion, sixth is the dither and the last one is image format.
    if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedBmp, sizeof(parrotSolderedBmp), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_BMP))
    //if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedJpg, sizeof(parrotSolderedJpg), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_JPG))
    //if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedPng, sizeof(parrotSolderedPng), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_PNG))
    {
        // If image decode failed, show error.
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        // Otherwise, just update the screen.
        inkplate.display();
    }
}

void loop()
{

}
