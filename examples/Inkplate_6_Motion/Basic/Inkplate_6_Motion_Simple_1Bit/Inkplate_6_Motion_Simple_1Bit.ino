// Include Inkplate Motion Arduino Library.
#include <InkplateMotion.h>

// Create an Inkplate object.
Inkplate inkplate;

// Variables for refresh time capture.
unsigned long time1 = 0;
unsigned long time2 = 0;

void setup()
{
    // Initialize serial communication (if Inkplate debug is used, must be added)!
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Inkplate Motion 6 Example started.");

    // Initialize Inkplate Motion Library in 1 bit mode.
    inkplate.begin(INKPLATE_1BW);

    // Disable set text wrap.
    inkplate.setTextWrap(false);

    // Randomase the seed for the radnom.
    randomSeed(analogRead(PA2));
    
    // Draw everything in the epaper framebuffer (calling this won't update the content on the screen).
    drawScreen(&inkplate);

    // Refresh the screen and capture the refresh time.
    time1 = millis();
    inkplate.display();
    time2 = millis();

    // Show refresh time on serial.
    Serial.print("Refresh time: ");
    Serial.print(time2 - time1, DEC);
    Serial.println("ms");
}

void loop()
{
    // Empty...
}

void drawScreen(Inkplate *_inkplate)
{
    // Print large letter "E" at the middle of the screen.
    _inkplate->setTextSize(80);
    int x = (_inkplate->width() - (80 * 6)) / 2;
    int y = (_inkplate->height() - (80 * 7)) / 2;
    _inkplate->setCursor(x, y);
    _inkplate->print('E');

    // Print the text.
    _inkplate->setTextSize(1);
    _inkplate->setTextColor(BLACK, WHITE);
    _inkplate->setCursor(50, 50);
    _inkplate->print("Hello, world!");

    // Fill the rect. Move it around with each refresh update - to see the ghosting effect.
    _inkplate->fillRect(200 + random(-50, 50), 200 + random(-50, 50), 600, 300, BLACK);

    // Draw diagonal line.
    _inkplate->drawLine(_inkplate->width() - 1, 0, 0, _inkplate->height(), BLACK);

    // Draw the border around the screen.
    _inkplate->drawRect(2, 2, _inkplate->width() - 4, _inkplate->height() - 4, BLACK);

    _inkplate->drawPixel(0, 0, BLACK);
    _inkplate->drawPixel(_inkplate->width() - 1, _inkplate->height() - 1, BLACK);
    _inkplate->drawPixel(0, _inkplate->height() - 1, BLACK);
    _inkplate->drawPixel(_inkplate->width() - 1, 0, BLACK);

    // Print small letters in the each corner of the screen.
    _inkplate->setCursor(0, 0);
    _inkplate->print("f");

    _inkplate->setCursor(0, 750);
    _inkplate->print("f");

    _inkplate->setCursor(1019, 0);
    _inkplate->print("e");

    _inkplate->setCursor(1019, 750);
    _inkplate->print("e");
}