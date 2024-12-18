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
    inkplate.begin(INKPLATE_1BW);
    //inkplate.begin(INKPLATE_GL16);

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
    //inkplate.image.draw("gradient.png", 0, 0);
    //inkplate.image.draw("rainbow.jpg", 0, 0);
    inkplate.image.draw("img1.jpg", 0, 0);
    //inkplate.image.draw("pexels-lauma-augstkalne-322733111-28681534.jpg", 0, 0);
    //inkplate.image.draw("pexels-njeromin-29233611.jpg", 0, 0);
    //inkplate.image.draw("pexels-helen1-29238664.jpg", 0, 0);
    //inkplate.image.draw("cat.jpg", 0, 0);
    //inkplate.image.draw("mountain.jpg", 0, 0);
    //inkplate.image.draw("road.jpg", 0, 0);
    //inkplate.image.draw("20241212_124918.jpg", 0, 0);
    //inkplate.image.draw("pexels-cottonbro-6862363.jpg", 0, 0);
    //inkplate.image.draw("pexels-chase-yaws-192435414-11513527.jpg", 0, 0);
    unsigned long time2 = micros();
    Serial.printf("DrawImage time: %lu\r\n", time2 - time1);

    // Now do a dither!
    time1 = micros();
    imgProcess.processImage((uint8_t*)(0xD0600000), 0, 0, 1024, 758, true, false, FS_KERNEL, FS_KERNEL_SIZE, inkplate.getDisplayMode() == INKPLATE_1BW?1:4);
    time2 = micros();
    Serial.printf("Dithering time: %lu\r\n", time2 - time1);
    inkplate.display();
}

void loop()
{
}