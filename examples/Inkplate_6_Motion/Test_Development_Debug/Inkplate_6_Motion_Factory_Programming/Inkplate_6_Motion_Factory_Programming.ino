/**
 **************************************************
 * @file        Inkplate_6_Motion_Factory_Programming.ino
 *
 * @brief       File for testing all features and initial programming of Inkplate 6 MOTION
 *
 * @note        !WARNING! VCOM can only be set 100 times, so keep usage to a minimum.
 *              !WARNING! Use at your own risk.
 *
 *              Below this header comment are test parameters which may be changed if required.
 *
 *              VCOM has to be set in memory and should always be -2.35V for 6MOTION panels.
 *              It is entered via Serial at baud 115200 with a - sign in from and a decimal point.
 *              So, for example, write "-2.35" with NL+CR when prompted to enter VCOM.
 *
 *              Tests will also be done, to pass all tests:
 *              - Edit the WiFi information below
 *              - Connect a follower device via EasyC on address 0x30 (or change the address below).
 *                In the InkplateEasyCTester folder, you can find the code for uploading to Dasduino Core
 *                or Dasduino ConnectPlus to convert Dasduino to an I2C follower device for testing an easyC connector
 *                if you don't have a device with address 0x30.
 *              - Insert a formatted microSD card (doesn't have to be empty)
 *              - Connect a Li-Ion Inkplate-compatible battery
 *              - Follow all the test's instructions
 *
 *              After all tests have passed the device will showcase the Inkplate 6MOTION onboarding image sequence.
 *
 *License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html Please review the
 *LICENSE file included with this example. If you have any questions about
 *licensing, please visit https://soldered.com/contact/ Distributed as-is; no
 *warranty is given.
 *
 * @authors     Soldered
 ***************************************************/

// Test parameters which may be changed if required:

// Uncomment this line to skip the VCOM step and just start doing tests
#define SKIP_VCOM

// If you want to write new VCOM voltage and perform change this number to something else
const int EEPROMoffset = 0;

// WiFi credentials for testing
char *wifiSSID = {"Soldered"};
char *wifiPASS = {"dasduino"};

// The easyC (I2C) follower address which will be checked
const uint8_t easyCDeviceAddress = 0x30;

// More detailed test parameters are available to edit in InkplateTest.cpp!

////////////////////////////////////////////////////////////////////////////////////////////////////

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(BOARD_INKPLATE6_MOTION)
#error "Wrong board selection for this example, please select Inkplate 6MOTION in the boards menu."
#endif

// Include Inkplate Motion library for STM32H743 MCU.
#include <InkplateMotion.h>

#include "InkplateTest.h"

// Sketch varaibles
Inkplate inkplate;      // Create an Inkplate Motion object.
double vcomVoltage;     // The entered voltage to use for VCOM
InkplateTest testClass; // The class which does all the testing

void setup()
{
    // Init Serial for communication
    Serial.begin(115200);

    // Init Inkplate class in 1-bit mode
    // This is required as a first step, to power everything up
    inkplate.begin(INKPLATE_1BW);
    delay(50); // Wait a bit

    // Init Inkplate test class (this just gives it pointer to the Inkplate object and params)
    testClass.init(&inkplate, EEPROMoffset, wifiSSID, wifiPASS, easyCDeviceAddress);

    // Write to Serial
    Serial.println("Inkplate 6MOTION test begin!");

    // Let's now manually test the TPS651851 e-Paper power controller
    Serial.println("Testing communication with TPS651851...");
    if (!testClass.tpsTest())
    {
        // Fatal error, don't continue the test - just inform the user via Serial
        Serial.println("Critical error: communication with TPS651851 failed!");
        Serial.println("Test stopping!");
        while (true)
            ;
    }
    Serial.println("TPS651851 OK!");

    // Let's now manually test the SDRAM
    Serial.println("Testing SDRAM...");
    if (!testClass.sdramTest())
    {
        // Fatal error, don't continue the test - just inform the user via Serial
        Serial.println("Critical error: SDRAM test failed!");
        Serial.println("Test stopping!");
        while (true)
            ;
    }
    Serial.println("SDRAM OK!");

    // Now, if SKIP_VCOM is NOT set, we need to enter VCOM
#ifndef SKIP_VCOM
    Serial.println("Setting VCOM...");
    if (!testClass.setVcom())
    {
        // Fatal error, don't continue the test - just inform the user via Serial
        Serial.println("Critical error: VCOM set failed!");
        Serial.println("Test stopping!");
        while (true)
            ;
    }
#endif

    // Now test everything else
    if (!testClass.testDevice())
    {
        // Fatal error, don't continue the test - just inform the user via Serial
        Serial.println("Critical error: one of the tests failed!");
        Serial.println("Test stopping!");
        // Also call display function to display the error
        // TODO
        while (true)
            ;
    }
}

void loop()
{
}