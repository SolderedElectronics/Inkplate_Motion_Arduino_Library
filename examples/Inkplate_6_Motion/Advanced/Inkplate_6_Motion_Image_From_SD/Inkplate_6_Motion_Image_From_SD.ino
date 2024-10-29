#include "InkplateMotion.h"

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

    // Try to initialize the card.
    if (!inkplate.microSDCardInit())
    {
        inkplate.println("microSD card init failed!");
        inkplate.partialUpdate();
        while(1);
    }
    else
    {
        inkplate.println("microSD Card Init ok! Image decode in progress, please wait!");
        inkplate.partialUpdate();
    }

    if (!inkplate.image.draw("image1.bmp", 0, 0, false, 0))
    //if (!inkplate.image.draw("image1.jpg", 0, 0, false, 0))
    //if (!inkplate.image.draw("image1.png", 0, 0, false, 0))
    //if (!inkplate.image.draw("image1_1", 0, 0, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_AUTO, INKPLATE_IMAGE_DECODE_PATH_SD))
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
