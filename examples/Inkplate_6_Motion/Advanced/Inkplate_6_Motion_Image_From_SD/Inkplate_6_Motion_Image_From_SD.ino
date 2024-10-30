// Incldue Inkplate Motion library.
#include "InkplateMotion.h"

// Create Inkplate Motion object.
Inkplate inkplate;

void setup()
{
    // Initialize serial communication.
    Serial.begin(115200);

    // Send message on serial so we know that Inkplate is alive.
    Serial.println("Hello from Inkplate 6 Motion!");
    Serial.flush();

    // Initialize Inkplate Motion Library. Use 4 bit mode.
    inkplate.begin(INKPLATE_GL16);
    inkplate.setTextColor(0, 15);
    inkplate.display();

    // Try to initialize the card.
    if (!inkplate.microSDCardInit())
    {
        // Show message on the screen if microSD init. failed.
        inkplate.println("microSD card init failed!");
        inkplate.partialUpdate();

        // Stop the code!
        while(1);
    }
    else
    {
        // If microSD init. was successful, display a message on the screen.
        inkplate.println("microSD Card Init ok! Image decode in progress, please wait!");
        inkplate.partialUpdate();
    }

    // Draw the image on the screen from microSD card. You can find the images in the sketch folder.
    // Method will try to find out if the path is local (microSD card) or Web and it will also try to
    // detected image fromat. If one of these fails, there are manual overrides.
    // First parameters is image path, second and third are image X and Y, forth is color invert, fifth is dither.
    // Two last parameters are optional - force image format and force path source.
    //if (!inkplate.image.draw("image1.bmp", 0, 0, false, 0))

    // Method can detect image format even if there is no extension.
    //if (!inkplate.image.draw("image1.jpg", 0, 0, false, 0))
    //if (!inkplate.image.draw("image1.png", 0, 0, false, 0))

    // But you can always override it.
    //if (!inkplate.image.draw("image1_1", 0, 0, false, 0, INKPLATE_IMAGE_DECODE_FORMAT_AUTO, INKPLATE_IMAGE_DECODE_PATH_SD))
    if (!inkplate.image.draw("image1.bmp", 0, 0, false, 0))
    {
        // Show erorr on the screen if decode failed.
        inkplate.println("Image decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        // Otherwise, refresh the screen.
        inkplate.display();
    }
}

void loop()
{

}
