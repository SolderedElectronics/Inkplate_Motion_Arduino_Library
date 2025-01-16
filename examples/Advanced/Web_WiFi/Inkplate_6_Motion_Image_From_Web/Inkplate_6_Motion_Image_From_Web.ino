/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_Image_From_Web.ino
 * @brief       Connect to Wi-Fi and download and show an image from the web
 *
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/

// Include Inkplate Motion library
#include <InkplateMotion.h>

Inkplate inkplate; // Create Inkplate object

// Change WiFi SSID and password here
#define WIFI_SSID "Soldered"
#define WIFI_PASS "dasduino"

// Link to download the .jpg image from
char imageUrl[] = {"https://gssc.esa.int/navipedia/images/a/a9/Example.jpg"};

void setup()
{
    inkplate.begin(INKPLATE_GRAYSCALE); // Initialize Inkplate in grayscale

    // Set text options
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(2);
    inkplate.setTextColor(0);
    inkplate.setTextWrap(true);

    // Let's initialize the Wi-Fi library:
    WiFi.init();

    // Set mode to Station
    WiFi.setMode(INKPLATE_WIFI_MODE_STA);

    // Connect to WiFi:
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait until we're connected, this is strongly reccomended to do!
    // At least do delay(3000) to give the modem some time to connect
    inkplate.print("Connecting to Wi-Fi...");
    while (!WiFi.connected())
    {
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    inkplate.println("\nSuccessfully connected to Wi-Fi!");
    inkplate.display();

    /*
    Let's draw the image from the URL
    First parameters is the URL
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

    // Note that progressive-encoded jpg's aren't supported
    if (!inkplate.image.draw(imageUrl, 0, 0, false, 1, FS_KERNEL, FS_KERNEL_SIZE))
    {
        // Show error on the screen if decode failed
        inkplate.println("Image download or decode failed!");
        inkplate.printf("Decode Err: %d\r\n", inkplate.image.getError());
        inkplate.partialUpdate();
    }
    else
    {
        // Otherwise, refresh the screen with a partial update
        inkplate.partialUpdate();
    }

    // Show the image on the display
    inkplate.display();
}

void loop()
{
}