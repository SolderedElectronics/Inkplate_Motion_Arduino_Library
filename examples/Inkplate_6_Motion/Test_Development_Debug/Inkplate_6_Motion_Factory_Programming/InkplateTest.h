#ifndef INKPLATETEST_H
#define INKPLATETEST_H

#include <InkplateMotion.h>

class InkplateTest
{
  public:
    // Initialization function
    void init(Inkplate *inkplateObj, const int EEPROMoffset, char *wifiSSID, char *wifiPASS,
              const uint8_t easyCDeviceAddress);

    // Set Vcom function
    bool setVcom();

    // Individual test functions
    bool displayUpdateTest();
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
    char *wifiSSID;
    char *wifiPASS;
    uint8_t easyCDeviceAddress;
    bool sdramTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint16_t _chunkSize);
    bool sdramChunkTestInternal(uint32_t _startAddress, uint32_t _endAddress, uint64_t *_writeSpeed,
                                uint64_t *_readSpeed, uint16_t *_failIndex);
    void printCurrentTestName(const char * testName);
    void printCurrentTestResult(bool _result);
    void printCurrentTestResult(bool _result, double _value);
    void draw4bitColorPalette();

};

#endif // INKPLATETEST_H