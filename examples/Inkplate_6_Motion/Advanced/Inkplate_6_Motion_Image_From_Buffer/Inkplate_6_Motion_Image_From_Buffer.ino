// Include Inkplate Motion Library.
#include "InkplateMotion.h"
#include "parrotSolderedBmp.h"
//#include "parrotSolderedJpg.h"
//#include "parrotSolderedPng.h"

Inkplate inkplate;

void setup()
{
    // Initialize serial communication.
    Serial.begin(115200);

    // Send message on serial so we know that Inkplate is alive.
    Serial.println("Hello from Inkplate 6 Motion!");
    Serial.flush();

    // Initialize Inkplate Motion Library.
    inkplate.begin(INKPLATE_GL16);
    inkplate.setTextColor(0, 15);
    inkplate.display();

    inkplate.println("loading image from the SRAM, please wait...");
    inkplate.partialUpdate();

    if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedBmp, sizeof(parrotSolderedBmp), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_BMP))
    //if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedJpg, sizeof(parrotSolderedJpg), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_JPG))
    //if (!inkplate.image.drawFromBuffer((uint8_t*)parrotSolderedPng, sizeof(parrotSolderedPng), 10, 10, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_PNG))
    {
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        inkplate.display();
    }
}

void loop()
{

}
