/**
 **************************************************
 * @file        Inkplate_6_Motion_VCOM_Set.ino
 *
 * @brief       File for initial programming of Inkplate 6MOTION
 *
 * @note        !WARNING! VCOM can only be set 100 times, so keep usage to a minimum.
 *
 *              VCOM has to be set in memory and should be -2.35V for 6MOTION panels.
 *              It is entered via Serial at baud 115200 with a - sign in from and a decimal point.
 *              So, for example, write "-2.35" with NL+CR when prompted to enter VCOM.
 *
 *              Some basic tests on the testing jig will also be done, see function testOnJig for more details
 *
 *License v3.0: https://www.gnu.org/licenses/lgpl-3.0.en.html Please review the
 *LICENSE file included with this example. If you have any questions about
 *licensing, please visit https://soldered.com/contact/ Distributed as-is; no
 *warranty is given.
 *
 * @authors     Robert @ Soldered
 ***************************************************/

// Test parameters which may be changed if required:

// If you want to write new VCOM voltage and perform change this number to something else
const int vcomEepromOffset = 0;

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
    inkplate.begin(INKPLATE_1BW);
    delay(800); // Wait a bit longer

    // Init Inkplate test class (this just gives it pointer to the Inkplate object and params)
    testClass.init(&inkplate, vcomEepromOffset, wifiSSID, wifiPASS, easyCDeviceAddress);


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

    // When prompted, enter VCOM via Serial with CRLF
    Serial.println("Setting VCOM...");
    if (!testClass.setVcom())
    {
        // Fatal error, don't continue the test - just inform the user via Serial
        Serial.println("Critical error: VCOM set failed!");
        Serial.println("Test stopping!");
        while (true)
            ;
    }

    // Wait for WAKE button press to start testing
    inkplate.setTextColor(1);
    inkplate.setTextSize(3);
    inkplate.setCursor(200, 300);
    inkplate.println("PRESS WAKE BUTTON TO BEGIN TESTS");
    inkplate.display();

    pinMode(INKPLATE_WAKE, INPUT_PULLUP);
    while (digitalRead(INKPLATE_WAKE) != LOW)
    {
        delay(1);
    }
    // Clear display!
    inkplate.clearDisplay();

    Serial.println("Doing basic tests...");
    if (!testClass.testOnJig())
    {
        // Fatal error happened, don't continue the test - just inform the user via Serial
        // Test result should also display on the e-Paper so this is just a precaution
        Serial.println("Critical error: one of the tests failed!");
        Serial.println("Test stopping!");
        // Go to infinite loop
        while (true)
            ;
    }

    // Inform the user that tests are complete and signal to continue tests
    inkplate.println(" ");
    inkplate.println("      TESTS  COMPLETE!");
    inkplate.println("      Press programming button to continue!");
    inkplate.display();
}

void loop()
{
    Serial.println("Continue");
    delay(1000);
}