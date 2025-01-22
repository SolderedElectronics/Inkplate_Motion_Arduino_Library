#include "InkplateTest.h"
#include "motion_testimg_1.h"
#include "motion_testimg_2.h"
#include "motion_testimg_3.h"
#include "motion_testimg_4.h"
#include <EEPROM.h>
#include <InkplateMotion.h>

// Here are some more detailed parameters of the test which can be changed like tolerances
// VCOM input
static const double minVcomInputOK = -5.0;
static const double maxVcomInputOK = -0.5;
// Battery voltage
static const double minBatteryVoltageOK = 3.2;
static const double maxBatteryVoltageOK = 4.5;
// WiFi timeout
static const int wifiTimeoutSeconds = 10;
// APDS9960 timeout
static const int apds9960testTimeoutSeconds = 10;
// APDS9960 interrupt flag and isr (interrupt also gets tested)
volatile bool apds9960isrFlag = false;
void ioExpanderISR()
{
    apds9960isrFlag = true;
}
// Gyroscope gets tested by taking float readings of the accelerometer
// and computing an acceleration vector magnitude
// Usually this value is around 9.81, some tolerance is added
static float accelVectorLowOK = 8;
static float accelVectorHighOK = 10.5;
// SHTC3 test parameters margins
// These are pretty large margins due to how the sensor operates
static const float degCLowOK = 10.0;
static const float degCHighOK = 45.0;
static const float humLowOK = 5.0;
static const float humHighOK = 95.0;

// String to be written to a file on the MicroSD card
const char sdCardTestStringLength = 100;
const char *testString = {"This is some test string..."};

// These are some variables for the SDRAM test
// Address to the SDRAM.
__IO uint8_t *ramBuffer = (__IO uint8_t *)0xD0000000;

// Array for test data for comparing SDRAM data. Use max size of 32768 Bytes for each.
__attribute__((section(".dma_buffer"))) volatile uint8_t _sourceArray[8192];
__attribute__((section(".dma_buffer"))) volatile uint8_t _compareArray[8192];

void InkplateTest::init(Inkplate *inkplateObj, const int EEPROMoffset, char *wifiSSID, char *wifiPASS,
                        const uint8_t qwiicTestAddress)
{
    this->inkplateObj = inkplateObj;
    this->EEPROMoffset = EEPROMoffset;
    this->wifiSSID = wifiSSID;
    this->wifiPASS = wifiPASS;
    this->qwiicTestAddress = qwiicTestAddress;

    // Set pinModes
    // These should have been previously set but set again just in case
    pinMode(INKPLATE_WAKE, INPUT_PULLUP);
    pinMode(INKPLATE_USER1, INPUT_PULLUP);
    pinMode(INKPLATE_USER2, INPUT_PULLUP);
}

bool InkplateTest::setVcom()
{
    // Set function local variables
    double vcom = -1.1; // Default value, won't be used
    char serialBuffer[50];
    unsigned long serialTimeout;

    // Let's wait for the user input
    while (true)
    {
        // Prompt user
        Serial.println("\nPlease write the VCOM voltage from e-paper panel!");
        Serial.println("Don't forget to use the negative (-) sign!");
        Serial.println("Use dot as the decimal point.");
        Serial.println("For example, '-1.23' for VCOM -1.23V.");
        Serial.println("Don't forget to use CRLF as the line ending!");
        Serial.println("Default Inkplate 6MOTION VCOM is -2.35V.");
        Serial.print("VCOM ranges from ");
        Serial.print(minVcomInputOK);
        Serial.print(" to ");
        Serial.println(maxVcomInputOK);
        Serial.println();
        Serial.print(">>>"); // This bit is important as it signals the upload script to send VCOM

        int i = 0;
        // Let's read the input
        while (true)
        {
            if (Serial.available())
            {
                char c = Serial.read();
                if (c == '\n' || c == '\r') // Stop reading on newline or carriage return
                    break;

                if (i < 49) // Avoid buffer overflow
                    serialBuffer[i++] = c;
            }
        }
        // Input read
        serialBuffer[i] = '\0'; // Null-terminate the string

        Serial.print("\nReceived: ");
        Serial.println(serialBuffer);

        // Let's do strtod conversion and check the output
        char *endPtr;
        vcom = strtod(serialBuffer, &endPtr);
        Serial.print("Parsed VCOM: ");
        Serial.println(vcom);
        // If endPtr points to the same address as serialBuffer, no conversion happened
        // If there's additional non-numeric data after the number, it's also invalid
        if (endPtr == serialBuffer || *endPtr != '\0')
        {
            Serial.println("Invalid input!\n");
            continue;
        }

        // Now check if VCOM is within reasonable values
        if (vcom <= minVcomInputOK || vcom >= maxVcomInputOK)
        {
            Serial.println("VCOM out of range!\n");
            continue;
        }

        Serial.println("VCOM in range OK!");
        // Now program VCOM, try several times
        bool vcomProgramResult = inkplateObj->pmic.programVCOM((float)vcom);
        if (!vcomProgramResult)
        {
            Serial.println("VCOM NOT set - error!");
            return false;
        }
        Serial.println("VCOM set!");

        // Check if VCOM written is OK
        Serial.print("Getting VCOM: ");
        delay(100);
        double getVcom = inkplateObj->pmic.getVCOM();
        Serial.println(getVcom);
        if (abs(getVcom - vcom) <= 0.2)
        {
            Serial.println("VCOM read back correctly!");
            inkplateObj->epdPSU(false);
            return true;
        }
        else
        {
            Serial.println("VCOM read back incorrectly!");
            inkplateObj->epdPSU(false);
            return false;
        }
    }

    return false; // This will never be reached
}

bool InkplateTest::tpsTest()
{
    bool result = false;

    // Turn on TPS651851
    int powerOnResult = inkplateObj->epdPSU(1);
    // This result variable also checks readPowerGood flag
    if (powerOnResult != 1)
    {
        // Something's wrong!
        // Couldn't power on!
        return result;
    }

    delay(10); // Wait a bit

    // Check if we can communicate with it and return that
    result = inkplateObj->pmic.begin();

    return result;
}

bool InkplateTest::sdramTest()
{
    // Call the internal function (which was taken from Inkplate_6_Motion_SDRAM_Test.ino example)
    return sdramTestInternal((uint64_t)ramBuffer, (uint64_t)(ramBuffer) + (32 * 1024 * 1024), 32768);
}

int InkplateTest::waitForUserInput()
{
    while (true)
    {
        if (digitalRead(INKPLATE_USER1) == LOW)
            return -1; // Previous slide
        if (digitalRead(INKPLATE_USER2) == LOW)
            return 1; // Next slide
        if (digitalRead(INKPLATE_WAKE) == LOW)
            return 0; // Exit
        delay(50);    // Debounce delay
    }
}


bool InkplateTest::displayUpdateTest()
{
    int currentTest = 0;
    const int totalTests = 8; // Total number of tests (excluding reset)

    while (true)
    {
        inkplateObj->clearDisplay();

        switch (currentTest)
        {
        case 0:
            inkplateObj->selectDisplayMode(INKPLATE_1BW);
            inkplateObj->setTextColor(BLACK);
            inkplateObj->setTextSize(3);
            inkplateObj->setCursor(50, 150);
            inkplateObj->setTextColor(1);
            inkplateObj->print("Test: 1bit full update");
            inkplateObj->display();
            break;

        case 1:
            inkplateObj->selectDisplayMode(INKPLATE_1BW);
            inkplateObj->setTextColor(BLACK);
            inkplateObj->fillRect(0, 0, inkplateObj->width(), inkplateObj->height(), 1);
            inkplateObj->setCursor(50, 150);
            inkplateObj->setTextColor(0);
            inkplateObj->print("Test: 1bit partial update");
            inkplateObj->partialUpdate();
            break;

        case 2:
            inkplateObj->selectDisplayMode(INKPLATE_GRAYSCALE);
            inkplateObj->setTextColor(0);
            inkplateObj->clearDisplay();
            inkplateObj->setCursor(50, 150);
            inkplateObj->print("Test: 4bit full update");
            draw4bitColorPalette();
            inkplateObj->display();
            break;

        case 3:
            inkplateObj->selectDisplayMode(INKPLATE_GRAYSCALE);
            inkplateObj->setTextColor(15);
            inkplateObj->fillRect(0, 0, inkplateObj->width(), inkplateObj->height(), 0);
            inkplateObj->setCursor(50, 150);
            inkplateObj->print("Test: 4bit partial update");
            draw4bitColorPalette();
            inkplateObj->partialUpdate();
            break;

        case 4:
            inkplateObj->selectDisplayMode(INKPLATE_1BW);
            inkplateObj->setTextColor(BLACK);
            inkplateObj->clearDisplay();
            inkplateObj->drawBitmap(0, 0, motion_testimg_1, inkplateObj->width(), inkplateObj->height(), BLACK);
            inkplateObj->display();
            break;

        case 5:
            inkplateObj->selectDisplayMode(INKPLATE_1BW);
            inkplateObj->setTextColor(BLACK);
            inkplateObj->clearDisplay();
            inkplateObj->drawBitmap(0, 0, motion_testimg_2, inkplateObj->width(), inkplateObj->height(), BLACK);
            inkplateObj->display();
            break;

        case 6:
            inkplateObj->selectDisplayMode(INKPLATE_GRAYSCALE);
            inkplateObj->setTextColor(15);
            inkplateObj->clearDisplay();
            inkplateObj->drawBitmap4Bit(0, 0, motion_testimg_3, inkplateObj->width(), inkplateObj->height());
            inkplateObj->display();
            break;

        case 7:
            inkplateObj->selectDisplayMode(INKPLATE_GRAYSCALE);
            inkplateObj->setTextColor(15);
            inkplateObj->clearDisplay();
            inkplateObj->drawBitmap4Bit(0, 0, motion_testimg_4, inkplateObj->width(), inkplateObj->height());
            inkplateObj->display();
            break;
        }

        int userInput = waitForUserInput();
        if (userInput == 0)
        {
            // Reset to 1bit mode before exiting
            inkplateObj->selectDisplayMode(INKPLATE_1BW);
            inkplateObj->clearDisplay();
            inkplateObj->display();
            inkplateObj->setCursor(0, 0);
            break;
        }
        currentTest = (currentTest + userInput + totalTests) % totalTests; // Loop through tests
    }

    return true;
}


bool InkplateTest::batteryVoltageReadTest()
{
    printCurrentTestName("Battery voltage");
    // Make reading
    double battRead = inkplateObj->readBattery();
    bool result = true; // Result is OK by default
    // Make check
    if (battRead >= maxBatteryVoltageOK || battRead <= minBatteryVoltageOK)
        result = false; // If out of bounds - test isn't passed
    // Print and return
    printCurrentTestResult(result, battRead);
    return result;
}

bool InkplateTest::wifiTest()
{
    printCurrentTestName("WiFi (Connecting...)");
    bool result = true;
    // Initialize ESP3 AT Commands Over SPI library.
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_WIFI, true);
    if (!WiFi.init())
    {
        // Something's wrong!
        result = false;
        // Print and return
        printCurrentTestResult(result);
        return result;
    }
    // Set wifi back to station mode
    WiFi.setMode(INKPLATE_WIFI_MODE_STA);
    // Now attempt to connect
    int counter = 0; // Counter for timeout
    WiFi.begin(wifiSSID, wifiPASS);
    delay(200); // Wait an initial time until WiFi connects
    // Maybe it connects on the first try, so this saves time
    while (!WiFi.connected())
    {
        // Count the second
        counter++;
        // Did we go over the limit?
        if (counter >= wifiTimeoutSeconds)
        {
            // Something's wrong!
            result = false;
            // Print and return
            printCurrentTestResult(result);
            return result;
        }
        delay(1000);
    }
    // We're connected!
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::rtcTest()
{
    printCurrentTestName("RTC");
    // Init RTC and check result
    inkplateObj->rtc.begin(RTC_HOURFORMAT_24);

    // Let's set a sample time and try to read it
    // Setting the clock: 12:24:25.
    uint8_t hours = 12;
    uint8_t minutes = 24;
    uint8_t seconds = 25;
    uint32_t subSeconds = 0;
    // Setting the date: 29/5/2024, Monday.
    uint8_t day = 29;
    uint8_t month = 5;
    uint8_t year = 24;
    // Recommended way of def. day of week.
    uint8_t weekday = RTC_WEEKDAY_MONDAY;

    // Set it!
    inkplateObj->rtc.setTime(hours, minutes, seconds, subSeconds);
    delay(5); // Wait a bit
    inkplateObj->rtc.setDate(day, month, year, weekday);
    delay(5); // Wait a bit

    // Wait a second...

    // Let's get back the time and date
    uint8_t h, m, s, d, mn, y, wk;
    uint32_t ss;
    inkplateObj->rtc.getTime(&h, &m, &s, &ss, NULL, NULL);
    inkplateObj->rtc.getDate(&d, &mn, &y, &wk);

    // Compare the set and retrieved values
    // Don't compare seconds as second might have passed in the meantime
    bool result = (h == hours) && (m == minutes) && (d == day) && (mn == month) && (y == year) && (wk == weekday);
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::microSdTest()
{
    printCurrentTestName("MicroSD");
    bool result = false;

    // Initialize microSD card
    if (!inkplateObj->microSDCardInit())
    {
        printCurrentTestResult(result);
        return result;
    }

    // Create and write to the file
    const char *testString = "Inkplate SD Card Test";
    File file = inkplateObj->sdFat.open("testFile.txt", O_CREAT | O_RDWR);
    if (!file)
    {
        printCurrentTestResult(result);
        return result;
    }
    file.print(testString);
    file.close();

    delay(250); // Wait a bit...

    // Read back the file
    file = inkplateObj->sdFat.open("testFile.txt", O_RDONLY);
    if (!file)
    {
        printCurrentTestResult(result);
        inkplateObj->sdFat.remove("testFile.txt"); // Ensure file is deleted
        return result;
    }

    char buffer[64];
    int bytesRead = file.read(buffer, sizeof(buffer) - 1);
    file.close();

    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        if (strcmp(buffer, testString) == 0)
        {
            result = true; // Test passed
        }
    }

    // Delete the test file
    inkplateObj->sdFat.remove("testFile.txt");

    printCurrentTestResult(result);
    return result;
}


bool InkplateTest::apds9960Test()
{
    printCurrentTestName("APDS9960 Gesture sensor (MAKE GESTURE - 10sec timeout!)");
    bool result = true;
    // Let's init APDS9960 like in the example
    // This will test interrupts as well
    // Set APDS INT pin on IO Expander as input. Override any GPIO pin protection.
    inkplateObj->internalIO.pinModeIO(IO_PIN_A0, INPUT, true);
    // Set interrupts on IO expander.
    inkplateObj->internalIO.setIntPinIO(IO_PIN_A0);
    // Enable interrptus on STM32.
    // NOTE: Must be set to change!
    attachInterrupt(digitalPinToInterrupt(PG13), ioExpanderISR, CHANGE);
    // Initialize APDS9960. Notify user if init has failed.
    if (!inkplateObj->apds9960.init())
    {
        // Something's wrong!
        result = false;
        // Print and return
        printCurrentTestResult(result);
        return result;
    }
    // Enable gesture sensor
    if (!inkplateObj->apds9960.enableGestureSensor(true))
    {
        // Something's wrong!
        result = false;
        // Print and return
        printCurrentTestResult(result);
        return result;
    }
    // Set gesture sensitivity level (higher the number = higher sensitivity).
    inkplateObj->apds9960.setGestureGain(0);
    // Clear previously read gesture
    inkplateObj->apds9960.readGesture();
    // Now wait for ISR flag to be changed with a timeout
    // Define timeout duration (in milliseconds)
    unsigned long timeoutMillis = apds9960testTimeoutSeconds * 1000;
    unsigned long startMillis = millis();

    // Now wait for ISR flag to be changed with a timeout
    while (!apds9960isrFlag)
    {
        if (millis() - startMillis >= timeoutMillis)
        {
            result = false; // Timeout occurred
            break;
        }
        delay(10); // Small delay to prevent busy waiting
    }
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::lsm6ds3Test()
{
    printCurrentTestName("LSM6DSO32 Gyroscope");
    bool result = true;
    // Power on
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_LSM6DSO32, true);
    delay(100); // Wait a bit to power on
    //  Get a new normalized sensor event
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    inkplateObj->lsm6dso32.getEvent(&accel, &gyro, &temp);
    float accelX = accel.acceleration.x;
    float accelY = accel.acceleration.y;
    float accelZ = accel.acceleration.z;
    // Let's check accelerometer vector magnitude magnitude to see if the device is functioning
    float accelMagnitude = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
    if (accelMagnitude < accelVectorLowOK || accelMagnitude > accelVectorHighOK)
    {
        // Out of bounds
        // Something's wrong!
        result = false;
    }

    // Power off
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_LSM6DSO32, false);
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::shtc3Test()
{
    printCurrentTestName("SHTC3 Temperature sensor");
    bool result = true;
    // Init sensor and take measurement
    inkplateObj->shtc3.begin();
    delay(10); // Wait a bit in case it needs time to start up
    // Also check if update was successful
    if (inkplateObj->shtc3.update() == SHTC3_Status_Nominal)
    {
        float degC = inkplateObj->shtc3.toDegC();
        float humidity = inkplateObj->shtc3.toPercent();
        // Compare and save to result
        result = (degC >= degCLowOK && degC <= degCHighOK && humidity >= humLowOK && humidity <= humHighOK);
    }
    else
    {
        // Update wasn't successful!
        result = false;
    }
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::rotaryEncTest()
{
    bool result = false;
    // Turn on the peripheral
    printCurrentTestName("Rotary encoder (Visual check & Spin - 10sec timeout!)");
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_ROTARY_ENCODER, true);
    // Initialize rotary encoder.
    inkplateObj->rotaryEncoder.begin();
    // Remember cursor for later
    int x = inkplateObj->getCursorX();
    int y = inkplateObj->getCursorY();

    delay(5);
    // Get the current position of the rotary encoder.
    int firstValue = (int)inkplateObj->rotaryEncoder.readAngle();
    // Set current angle as zero
    inkplateObj->rotaryEncoder.setOffset(firstValue);
    unsigned long startTime = millis();
    while (millis() - startTime < 10000)
    {
        // Get the current rotary encoder value
        int currentValue = (int)inkplateObj->rotaryEncoder.readAngle();
        int difference = abs(currentValue - firstValue);
        // Print the new value in the same place
        inkplateObj->setCursor(900, 320);
        inkplateObj->fillRect(880, 300, 500, 500, 0);
        inkplateObj->print(difference);
        inkplateObj->partialUpdate(true);
        delay(150); // Small delay
    }

    // Turn off
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_ROTARY_ENCODER, false);
    // Bring back the cursor
    inkplateObj->setCursor(x, y);
    // Remove numerical print
    inkplateObj->fillRect(880, 300, 500, 500, 0);
    result = true; // This test will always pass as some Inkplates aren't tested in enclosure
    // It's intended to be a visual check
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::qwiicTest()
{
    printCurrentTestName("Qwiic (I2C)");
    bool result = false;
    Wire.begin();
    Wire.beginTransmission(qwiicTestAddress);
    if (Wire.endTransmission() == 0)
        result = true;
    printCurrentTestResult(result);
    return result;
}

bool InkplateTest::buttonPressTest()
{
    printCurrentTestName("Buttons - PRESS WAKE, USER1 and USER2 - 10sec timeout!\n");

    bool result = false;

    // Start measuring time
    unsigned long startTime = millis();
    bool wakePressed = false;
    bool user1Pressed = false;
    bool user2Pressed = false;

    // Keep checking until timeout or all buttons are pressed
    while (millis() - startTime < 10000)
    {
        if (!wakePressed && digitalRead(INKPLATE_WAKE) == LOW)
        {
            wakePressed = true;
            inkplateObj->print("WAKE OK! ");
            inkplateObj->partialUpdate();
        }

        if (!user1Pressed && digitalRead(INKPLATE_USER1) == LOW)
        {
            user1Pressed = true;
            inkplateObj->print("USER1 OK! ");
            inkplateObj->partialUpdate();
        }

        if (!user2Pressed && digitalRead(INKPLATE_USER2) == LOW)
        {
            user2Pressed = true;
            inkplateObj->print("USER2 OK! ");
            inkplateObj->partialUpdate();
        }

        // If all buttons have been pressed, set result to true and break out of the loop
        if (wakePressed && user1Pressed && user2Pressed)
        {
            result = true;
            break;
        }
    }
    printCurrentTestResult(result);
    return result;
}


bool InkplateTest::testOnJig()
{
    // Do tests of screen updates
    displayUpdateTest();
    // Print several newlines on the display
    // This will be displayed with the alignment check in the background
    inkplateObj->println("");
    inkplateObj->println("");
    inkplateObj->println("");
    // Do visual check of panel:
    checkScreenBorder();
    // Now do the rest of the tests
    if (!qwiicTest())
        return false;
    if (!batteryVoltageReadTest())
        return false;
    if (!microSdTest())
        return false;
    if (!rtcTest())
        return false;
    if (!wsLedTest())
        return false;
    if (!lsm6ds3Test())
        return false;
    if (!shtc3Test())
        return false;
    return true;
}

bool InkplateTest::testInEnclosure()
{
    // Print a newline on the display so that it's more readable
    inkplateObj->println("");
    // Now do the rest of the tests
    if (!wifiTest())
        return false;
    if (!rtcTest())
        return false;
    if (!wsLedTest())
        return false;
    if (!lsm6ds3Test())
        return false;
    if (!shtc3Test())
        return false;
    if (!apds9960Test())
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

void InkplateTest::printCurrentTestName(const char *_testName)
{
    // Make sure text size and color are set correctly
    inkplateObj->setTextSize(2);
    inkplateObj->setTextColor(1);
    // Using printLN to automatically go to newline
    inkplateObj->print("    -");
    inkplateObj->print(_testName);
    inkplateObj->print(": ");
    // Update it
    inkplateObj->partialUpdate();
}

void InkplateTest::printCurrentTestResult(bool _result)
{
    if (_result)
    {
        inkplateObj->println("OK!");
    }
    else
    {
        inkplateObj->println("FAIL!");
        inkplateObj->println("");
        inkplateObj->println("    Stopping test!");
    }
    // Update it
    inkplateObj->partialUpdate();
}

void InkplateTest::printCurrentTestResult(bool _result, double _value)
{
    inkplateObj->print(_value);
    if (_result)
    {
        inkplateObj->println(" OK!");
    }
    else
    {
        inkplateObj->println(" FAIL!");
    }
    // Update it
    inkplateObj->partialUpdate();
}

void InkplateTest::draw4bitColorPalette()
{
    for (int color = 0; color < 16; ++color)
    {
        // Draw each color block
        inkplateObj->fillRect(50 + (color * 50), 400, 50, 300, color);
    }
}

bool InkplateTest::areTestsDone(int eepromOffset, int expectedValue)
{
    // Read the value from EEPROM at the given offset
    int storedValue = EEPROM.read(eepromOffset);
    // Return true if the stored value matches the expected value
    return storedValue == expectedValue;
}

void InkplateTest::writeEepromValue(int eepromOffset, int valueToWrite)
{
    // Write the given value to the specified EEPROM offset
    EEPROM.write(eepromOffset, valueToWrite);
}

void InkplateTest::checkScreenBorder()
{
    printCurrentTestName("Check screen borders! (visual check)");

    // Size of small rectangles in the corners of the screen
    int _smallRectSize = 11;

    // Draw a black rectangle from edge to edge of the screen
    inkplateObj->drawRect(0, 0, inkplateObj->width(), inkplateObj->height(), BLACK);

    // Now draw 4 small rectangles in all four corners of the screen
    inkplateObj->drawRect(0, 0, _smallRectSize, _smallRectSize, BLACK);
    inkplateObj->drawRect(inkplateObj->width() - _smallRectSize, 0, _smallRectSize, _smallRectSize, BLACK);
    inkplateObj->drawRect(0, inkplateObj->height() - _smallRectSize, _smallRectSize, _smallRectSize, BLACK);
    inkplateObj->drawRect(inkplateObj->width() - _smallRectSize, inkplateObj->height() - _smallRectSize, _smallRectSize,
                          _smallRectSize, BLACK);

    // Send image to the screen
    inkplateObj->partialUpdate();

    // Wait a little bit
    delay(2000);
    // This test always passes
    printCurrentTestResult(true);
}

bool InkplateTest::wsLedTest()
{
    // This test will always succeed - it's a visual check
    printCurrentTestName("WS2812 RGB LED's - Visual check");
    bool result = true;
    // Enable LEDs and init
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_WS_LED, true);
    delay(5);
    inkplateObj->led.begin();
    delay(5);
    // Set medium brightness
    inkplateObj->led.setBrightness(100);

    // Cycle some colors
    inkplateObj->led.setPixelColor(0, 150, 0, 0);
    inkplateObj->led.setPixelColor(1, 150, 0, 0);
    inkplateObj->led.show();
    delay(500);
    inkplateObj->led.setPixelColor(0, 0, 150, 0);
    inkplateObj->led.setPixelColor(1, 0, 150, 0);
    inkplateObj->led.show();
    delay(500);
    inkplateObj->led.setPixelColor(0, 0, 0, 150);
    inkplateObj->led.setPixelColor(1, 0, 0, 150);
    inkplateObj->led.show();
    delay(500);
    inkplateObj->led.setPixelColor(0, 150, 150, 150);
    inkplateObj->led.setPixelColor(1, 150, 150, 150);
    inkplateObj->led.show();
    delay(500);


    // Disable LEDs
    inkplateObj->peripheralState(INKPLATE_PERIPHERAL_WS_LED, false);

    printCurrentTestResult(result);
    return result;
}