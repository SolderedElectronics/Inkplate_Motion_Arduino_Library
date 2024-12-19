// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create Inkplate Motion object.
Inkplate inkplate;

void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send hello message on serial, so we know the Inkplate board is alive.
    Serial.println("Hello from Inkplate 6 Motion!");

    // Initialize Inkplate Motion library in 4 bit mode.
    inkplate.begin(INKPLATE_GL16);

    // Clear the screen.
    inkplate.display();

    // Set GFX parameters.
    inkplate.setTextColor(0, 15);
    inkplate.setTextSize(3);
    inkplate.setCursor(0, 0);

    // Enable the microSD card peripheral.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_MICROSD, true);
    
    // Try to initialize the card.
    if (!inkplate.microSDCardInit())
    {
        // Stop the code if initalization has failed.
        Serial.println("MicroSD card init failed!\r\n");
        while(1);
    }
    else
    {
        Serial.println("MicroSD card init ok!\r\n");
    }

    // Try to load the image from microSD card using Floyd–Steinberg dithering.
    // - First parameter is the path/filename.
    // - Second and third parameters are X and Y cooridinates where image should be drawn on the screen.
    // - Forth parameter is to switch for color inversion on drawn image.
    // - Fifth parameter is switch to enable or disable dithering.
    // - Sixth and seventh parameters are for dither kernels. Use NULL and 0 if dithering is not used.
    // - Eighth parameter is for image format. By default is set to AUTO. This means the library will try
    // to automatically detect image format.
    // - Ninth parameter will set the source (WEB or microSD card). By default is set to auto.
    if (!inkplate.image.draw("img1.jpg", 0, 0, false, true, FS_KERNEL, FS_KERNEL_SIZE))
    {
        inkplate.printf("Load image failed, error code %d. Code halt!\r\n", inkplate.image.getError());
        inkplate.display();
    }

    // Display image on the screen.
    inkplate.display();

    // Wait a little bit.
    delay(2500);

    // Now load the same image with other dither kernel.
    inkplate.clearDisplay();
    if (!inkplate.image.draw("img1.jpg", 0, 0, false, true, STUCKI_KERNEL, STUCKI_KERNEL_SIZE))
    {
        inkplate.printf("Load image failed, error code %d. Code halt!\r\n", inkplate.image.getError());
        inkplate.display();
    }

    // Display image on the screen.
    inkplate.display();

    // Wait a little bit.
    delay(2500);

    // Now switch the modes and use 1 bit mode dithering. Now we're going to use Sierra kernel.
    inkplate.selectDisplayMode(INKPLATE_1BW);
    if (!inkplate.image.draw("img1.jpg", 0, 0, false, true, SIERRA_KERNEL, SIERRA_KERNEL_SIZE))
    {
        inkplate.printf("Load image failed, error code %d. Code halt!\r\n", inkplate.image.getError());
        inkplate.display();
    }

    // Display image on the screen.
    inkplate.display();

    // Wait a little bit.
    delay(2500);

    // Last one, invert the colors!
    if (!inkplate.image.draw("img1.jpg", 0, 0, true, true, SIERRA_KERNEL, SIERRA_KERNEL_SIZE))
    {
        inkplate.printf("Load image failed, error code %d. Code halt!\r\n", inkplate.image.getError());
        inkplate.display();
    }

    // Display image on the screen.
    inkplate.display();

    // Wait a little bit.
    delay(2500);
}

void loop()
{
    // Empty...
}