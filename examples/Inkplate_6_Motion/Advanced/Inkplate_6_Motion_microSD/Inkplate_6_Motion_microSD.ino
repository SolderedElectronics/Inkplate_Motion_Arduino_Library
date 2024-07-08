// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create Inkplate Motion object.
Inkplate inkplate;

void setup()
{
    // Initialize Serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Hello from Inkplate 6 Motion!");

    // Initialize Inkplate Motion library.
    inkplate.begin();

    // Power up the card!
    inkplate.internalIO.pinModeIO(PERIPHERAL_SD_ENABLE_PIN, OUTPUT, true);
    delay(500);

    // Try to init the card!
    if (!inkplate.microSD.sdCardInit())
    {
        Serial.println("microSD card init failed, code halt!");

        while(1);
    }
    else
    {
        Serial.println("microSD init ok!");
    }

    inkplate.microSD.sd.ls();

    File file = inkplate.microSD.sd.open("i2c_recv.c", O_RDONLY);

    if (!file)
    {
        Serial.println("Open failed!");
    }
    else
    {
        uint32_t n = file.available();
        while (n)
        {
            uint8_t buffer[4096];

            uint32_t _size = n > 4096?4096:n;

unsigned long timestamp1 = micros();

            file.read(buffer, _size);
unsigned long timestamp2 = micros();
Serial.println(timestamp2 - timestamp1, DEC);
Serial.println(_size, DEC);

            n-=_size;
            
            for (int i = 0; i < _size; i++)
            {
                Serial.write(buffer[i]);
            }
        }
        file.close();
    }
    
}

void loop()
{
    
}