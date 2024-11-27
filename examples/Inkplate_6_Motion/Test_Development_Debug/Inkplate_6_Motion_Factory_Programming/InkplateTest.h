#ifndef INKPLATETEST_H
#define INKPLATETEST_H

#include <InkplateMotion.h>

class InkplateTest {
public:
    // Initialization function
    void init(Inkplate *inkplateObj, const int EEPROMoffset, const char *wifiSSID, const char *wifiPASS, const uint8_t easyCDeviceAddress);

    // Set Vcom function
    bool setVcom(double vCom);

    // Individual test functions
    bool tpsTest();
    bool sdramTest();
    bool batteryVoltageReadTest();
    bool wifiTest();
    bool rtcTest();
    bool microSdTest();
    bool apds9960Test();
    bool lsm6ds3Test();
    bool shtc3Test();
    bool rotaryEncTest();
    bool buttonPressTest();

    // Test all functions in series
    bool testDevice();

private:
    Inkplate *inkplateObj;
    int EEPROMoffset;
    const char *wifiSSID;
    const char *wifiPASS;
    uint8_t easyCDeviceAddress;
    bool sdramTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint16_t _chunkSize);
    bool sdramChunkTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint64_t *_writeSpeed, uint64_t *_readSpeed, uint16_t *_failIndex);
};

#endif // INKPLATETEST_H