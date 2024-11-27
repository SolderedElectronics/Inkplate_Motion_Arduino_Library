#include "InkplateTest.h"
#include <InkplateMotion.h>

// These are some variables for the SDRAM test
// Address to the SDRAM.
__IO uint8_t *ramBuffer = (__IO uint8_t *)0xD0000000;

// Array for test data for comparing SDRAM data. Use max size of 32768 Bytes for each.
__attribute__((section(".dma_buffer"))) volatile uint8_t _sourceArray[8192];
__attribute__((section(".dma_buffer"))) volatile uint8_t _compareArray[8192];

void InkplateTest::init(Inkplate *inkplateObj, const int EEPROMoffset, const char *wifiSSID, const char *wifiPASS,
                        const uint8_t easyCDeviceAddress)
{
    this->inkplateObj = inkplateObj;
    this->EEPROMoffset = EEPROMoffset;
    this->wifiSSID = wifiSSID;
    this->wifiPASS = wifiPASS;
    this->easyCDeviceAddress = easyCDeviceAddress;
}

bool InkplateTest::setVcom(double vCom)
{
    return true;
}

bool InkplateTest::tpsTest()
{
    // Turn on TPS651851
    inkplateObj->epdPSU(1);

    delay(10); // Wait a bit

    // Check if we can communicate with it - this is the test
    bool result = inkplateObj->pmic.begin();

    // Turn it back off
    inkplateObj->epdPSU(0);

    delay(10); // Wait a bit

    return result;
}


bool InkplateTest::sdramTest()
{
    // Call the internal function (which is gotten from Inkplate_6_Motion_SDRAM_Test.ino example)
    return sdramTestInternal((uint64_t)ramBuffer, (uint64_t)(ramBuffer) + (32 * 1024 * 1024), 32768);
}
bool InkplateTest::batteryVoltageReadTest()
{
    return true;
}
bool InkplateTest::wifiTest()
{
    return true;
}
bool InkplateTest::rtcTest()
{
    return true;
}
bool InkplateTest::microSdTest()
{
    return true;
}
bool InkplateTest::apds9960Test()
{
    return true;
}
bool InkplateTest::lsm6ds3Test()
{
    return true;
}
bool InkplateTest::shtc3Test()
{
    return true;
}
bool InkplateTest::rotaryEncTest()
{
    return true;
}
bool InkplateTest::buttonPressTest()
{
    return true;
}

bool InkplateTest::testDevice()
{
    if (!batteryVoltageReadTest())
        return false;
    if (!wifiTest())
        return false;
    if (!rtcTest())
        return false;
    if (!microSdTest())
        return false;
    if (!apds9960Test())
        return false;
    if (!lsm6ds3Test())
        return false;
    if (!shtc3Test())
        return false;
    if (!rotaryEncTest())
        return false;
    if (!buttonPressTest())
        return false;
    return true;
}

bool InkplateTest::sdramTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint16_t _chunkSize)
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
        _testOK =
            sdramChunkTestInternal(_startChunkAddress, _endAddressChunk, &_writeSpeed, &_readSpeed, &_failedIndex);
        // Print out the results.
        if (!_testOK)
        {
            Serial.printf("SDRAM Failed @ 0x%08X\r\n", _startChunkAddress + _failedIndex);
            Serial.flush();
            // If any of the tests failed return false
            return false;
        }

        // Decrement the remain lenght.
        _len -= _chunkLen;

        // Increment start chunk address.
        _startChunkAddress += _chunkLen;
    }

    // If this point in code was reached, everything went OK
    return true;
}

bool InkplateTest::sdramChunkTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint64_t *_writeSpeed,
                                          uint64_t *_readSpeed, uint16_t *_failIndex)
{

    stm32FmcGetSdramMdmaInstance();

    // Handle for Master DMA for SDRAM.
    MDMA_HandleTypeDef hmdmaMdmaChannel40Sw0 = *stm32FmcGetSdramMdmaInstance();

    // Handle for the SDRAM FMC interface.
    SDRAM_HandleTypeDef hsdram1 = *stm32FmcGetSdramInstance();

    // Calculate the length of the test array.
    uint16_t _len = _endAddress - _startAddress;

    // Check if the test area will fit inside the DMA buffer,
    // If not, return with fail.
    if (_len > 8192)
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