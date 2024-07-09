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

    // Set text scale to 3.
    inkplate.setTextSize(3);

    // Try to init the card!
    if (!inkplate.microSDCardInit())
    {
        inkplate.println("microSD card init failed, code halt!");
        inkplate.partialUpdate();

        while(1);
    }
    else
    {
        inkplate.println("microSD init ok!");
        inkplate.partialUpdate();
    }

    File file = inkplate.sdFat.open("i2c_recv.c", O_RDONLY);

    if (!file)
    {
        inkplate.println("Open failed!");
        inkplate.partialUpdate();
    }
    else
    {
        inkplate.setTextSize(1);
        uint32_t n = file.available();
        while (n)
        {
            uint8_t buffer[4096];
            uint32_t _size = n > 4096?4096:n;
            file.read(buffer, _size);
            n-=_size;
            
            for (int i = 0; i < _size; i++)
            {
                inkplate.write(buffer[i]);
            }
        }
        file.close();
        inkplate.partialUpdate();
    }
    
}

void loop()
{
    
}