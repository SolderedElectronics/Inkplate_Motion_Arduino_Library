#include <InkplateMotion.h>

#include "image1.h"
#include "image2.h"

Inkplate inkplate;

void setup()
{
    Serial.begin(115200);
    Serial.println("STM32 code started...");

    inkplate.begin(INKPLATE_GL16);

    // pinMode(PG9, OUTPUT);
    // digitalWrite(PG9, LOW);

    drawGrad(&inkplate, 0, 300, inkplate.width(), 200);
    inkplate.display();
    delay(2500);

    inkplate.clearDisplay();
    inkplate.drawRect(0, 0, inkplate.width(), inkplate.height(), 0);
    inkplate.drawRect(0, 0, 5, 5, 0);
    inkplate.drawRect(inkplate.width() - 5, 0, 5, 5, 0);
    inkplate.drawRect(0, inkplate.height() - 5, 5, 5, 0);
    inkplate.drawRect(inkplate.width() - 5, inkplate.height() - 5, 5, 5, 0);
    inkplate.display();
    delay(2500);

    inkplate.clearDisplay();
    inkplate.drawBitmap4Bit(0, 0, img1, img1_w, img1_h);
    inkplate.display();
    delay(2500);

    inkplate.clearDisplay();
    inkplate.drawBitmap4Bit(0, 0, img2, img2_w, img2_h);
    inkplate.display();
    delay(2500);
}

void loop()
{

}

void drawGrad(Inkplate *_inkplate, int _x, int _y, int _w, int _h)
{
    int _xStep = _w / 16;

    for (int i = 0; i < 16; i++)
    {
        _inkplate->fillRect(_x + (_xStep * i), _y, _xStep, _h, i);
    }
}