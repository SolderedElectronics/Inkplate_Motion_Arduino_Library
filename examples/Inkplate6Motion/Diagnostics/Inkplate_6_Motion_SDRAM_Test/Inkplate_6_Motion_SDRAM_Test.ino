/**
 **************************************************
 *
 * @file        Inkplate_6_Motion_SDRAM_Test.ino
 * @brief       Test the onboard SDRAM
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
 ***************************************************/
// Include Inkplate Motion library for STM32H743 MCU.
#include <InkplateMotion.h>

// Create an Inkplate Motion object.
Inkplate inkplate;

// Address to the SDRAM.
__IO uint8_t *ramBuffer = (__IO uint8_t *)0xD0000000;

// Handle for Master DMA for SDRAM.
extern MDMA_HandleTypeDef hmdmaMdmaChannel40Sw0;

// Handle for the SDRAM FMC interface.
extern SDRAM_HandleTypeDef hsdram1;

// Array for test data for comparing SDRAM data. Use max size of 32768 Bytes for each.
__attribute__((section(".dma_buffer"))) volatile uint8_t _sourceArray[32768];
__attribute__((section(".dma_buffer"))) volatile uint8_t _compareArray[32768];

void setup()
{
    // Serial debugging.
    Serial.begin(115200);
    Serial.printf("Inkplate 6 Motion code is starting...");

    // Initialize the library (needed for the SDRAM and MDMA).
    inkplate.begin(INKPLATE_GL16);

    // Run the test! NOTE: Chunk size must be less then 32768 bytes!
    // testSDRAM(_ramStartAddress, _ramEndAddress);
    testSDRAM((uint64_t)ramBuffer, (uint64_t)(ramBuffer) + (32 * 1024 * 1024), 32768);
}

void loop()
{
    // Empty...
}

void testSDRAM(uint32_t _startAddress, uint32_t _endAddress, uint16_t _chunkSize)
{
    // Calculate the lenght of the test area of the SDRAM.
    uint64_t _len = _endAddress - _startAddress;

    // Variable to store the success of the chunk test.
    bool _testOK = true;

    uint64_t _startChunkAddress = _startAddress;
    uint16_t _failedIndex = 0;

    // Loop until test gone thorugh whole SDRAM or SDRAM test failed.
    while (_testOK && _len > 0)
    {
        uint64_t _writeSpeed = 0;
        uint64_t _readSpeed = 0;

        // Calculate the the lenght of the chunk.
        uint16_t _chunkLen = _len > _chunkSize ? _chunkSize : _len;

        // Calculate the end address of the chunk.
        uint64_t _endAddressChunk = _startChunkAddress + _chunkLen;

        // Test the chunk of the memory.
        Serial.printf("Testing Address Range 0x%08X - ", _startChunkAddress);
        Serial.printf("0x%08X: ", _endAddressChunk);
        Serial.flush();
        _testOK = testSDRAMChunk(_startChunkAddress, _endAddressChunk, &_writeSpeed, &_readSpeed, &_failedIndex);
        // Print out the results.
        if (_testOK)
        {
            Serial.printf("OK! R: %luMB/s ", _readSpeed / (1024ULL * 1024ULL));
            Serial.printf("W: %lu MB/s\r\n", _writeSpeed / (1024ULL * 1024ULL));
            Serial.flush();
        }
        else
        {
            Serial.printf("Failed @ 0x%08X\r\n", _startChunkAddress + _failedIndex);
            Serial.flush();
        }

        // Decrement the remain lenght.
        _len -= _chunkLen;

        // Increment start chunk address.
        _startChunkAddress += _chunkLen;
    }
}

bool testSDRAMChunk(uint32_t _startAddress, uint32_t _endAddress, uint64_t *_writeSpeed, uint64_t *_readSpeed,
                    uint16_t *_failIndex)
{
    // Calculate the length of the test array.
    uint16_t _len = _endAddress - _startAddress;

    // Check if the test area will fit inside the DMA buffer,
    // If not, return with fail.
    if (_len > 32768)
        return true;

    // Variables for calculating R/W speed.
    unsigned long _startTime;
    unsigned long _endTime;

    // Shuffle the radnom seed.
    randomSeed(analogRead(PA6));

    // Fill the array with random 8 bit data.
    for (int i = 0; i < _len; i++)
    {
        _sourceArray[i] = random(0, 255);
    }

    // Capture the time!
    _startTime = micros();

    stm32FmcGetSdramMdmaInstance();

    // Handle for Master DMA for SDRAM.
    MDMA_HandleTypeDef hmdmaMdmaChannel40Sw0 = *stm32FmcGetSdramMdmaInstance();

    // Send that data to the SDRAM with DMA!
    HAL_MDMA_Start(&hmdmaMdmaChannel40Sw0, (uint32_t)_sourceArray, (uint32_t)_startAddress, _len, 1);

    // Wait for transfer to complete.
    HAL_MDMA_PollForTransfer(&hmdmaMdmaChannel40Sw0, HAL_MDMA_FULL_TRANSFER, 1000ULL);

    // Capture it!
    _endTime = micros();

    // Calculate the speed.
    if (_writeSpeed != NULL)
    {
        *_writeSpeed = (uint64_t)(1.0 / ((_endTime - _startTime) * 1E-6) * _len);
    }

    // Wait a little bit.
    delay(20);

    // Capture the time!
    _startTime = micros();

    // Now, read back the data.
    HAL_MDMA_Start(&hmdmaMdmaChannel40Sw0, (uint32_t)_startAddress, (uint32_t)_compareArray, _len, 1);

    // Wait for transfer to complete.
    HAL_MDMA_PollForTransfer(&hmdmaMdmaChannel40Sw0, HAL_MDMA_FULL_TRANSFER, 1000ULL);

    // Capture it!
    _endTime = micros();

    // Calculate the speed.
    if (_readSpeed != NULL)
    {
        *_readSpeed = (uint64_t)(1.0 / ((_endTime - _startTime) * 1E-6) * _len);
    }

    // Compare it.
    for (int i = 0; i < _len; i++)
    {
        if (_sourceArray[i] != (_compareArray[i]))
        {
            if (_failIndex != NULL)
                *_failIndex = i;
            return false;
        }
    }

    // Everything went ok? Return true!
    return true;
}
