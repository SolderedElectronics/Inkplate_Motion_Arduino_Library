/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_Accelerometer_Cube.ino
 * @brief       This example will show you how to project a 3D cube to Inkplate and rotate it with data from the
 *accelerometer
 *
 *              To successfully run the sketch:
 *              -Connect Inkplate 6 MOTION via USB-C cable
 *              -Press the programming button to put the device in the programming state
 *              -Upload the code
 *
 * @see         solde.red/333321
 *
 * @authors     Robert @ soldered.com
 * @date        July 2024
 ***************************************************/

// Include the Inkplate Motion library
#include <InkplateMotion.h>

Inkplate inkplate; // Create an Inkplate object

// Variables which are used for drawing the 3D Cube
// Cube vertices
float cube[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
// Cube edges
int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges
};
// This value multiplies the accelerometer readings to help project the cube in the orientation of the accelerometer
// If you want accelerometer movements to have more effect on the cube's retation, increase this
// And vice versa
#define ANGLE_MODIFIER 0.0012

// Variables for the angles at which the cube gets projected
float angleX = 0;
float angleY = 0;
float angleZ = 0;
// Also, remember the previous angles
// This is just to calculate the average between the two in order to smooth out the movement
float previousAngleX = 0;
float previousAngleY = 0;
float previousAngleZ = 0;

uint32_t previousTime;

// Setup code, runs only once
void setup()
{
    inkplate.begin();   // Init Inkplate library (you should call this function ONLY ONCE)
    inkplate.display(); // Put clear image on display

    // Set text size to be 2x larger than default (5x7px)
    inkplate.setTextSize(2);
    inkplate.setTextColor(BLACK); // Set the text color to black also

    // Turn on the LSM6DS3 peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_LSM6DS3, true);
    delay(1000); // Wait a bit

    // Try to init the acccelerometer
    int result = inkplate.lsm6ds3.begin();

    // Set 75 frames of partial updates for a full update
    // Looks okay on this example
    inkplate.setFullUpdateTreshold(75);

    previousTime = millis();
}

void loop()
{
    // First, clear what was previously in the frame buffer
    inkplate.clearDisplay();

    // Read values from the accelerometer (for display purposes)
    float accelX = inkplate.lsm6ds3.readRawAccelX();
    float accelY = inkplate.lsm6ds3.readRawAccelY();
    float accelZ = inkplate.lsm6ds3.readRawAccelZ();

    // Read values from the gyroscope
    float gyroX = inkplate.lsm6ds3.readFloatGyroX() / 18.5;
    float gyroY = inkplate.lsm6ds3.readFloatGyroY() / 18.5;
    float gyroZ = inkplate.lsm6ds3.readFloatGyroZ() / 18.5;

    // Print accelerometer readings on the display
    inkplate.setCursor(10, 630);
    inkplate.print("ACC X:");
    inkplate.print(accelX, 4);
    inkplate.setCursor(10, 650);
    inkplate.print("ACC Y:");
    inkplate.print(accelY, 4);
    inkplate.setCursor(10, 670);
    inkplate.print("ACC Z:");
    inkplate.print(accelZ, 4);

    // Print gyroscope readings on the display also
    inkplate.setCursor(10, 690);
    inkplate.print("GYRO X:");
    inkplate.print(gyroX, 4);
    inkplate.setCursor(10, 710);
    inkplate.print("GYRO Y:");
    inkplate.print(gyroY, 4);
    inkplate.setCursor(10, 730);
    inkplate.print("GYRO Z:");
    inkplate.print(gyroZ, 4);

    // Calculate the time difference since the last loop
    unsigned long currentTime = millis();
    float dt = (currentTime - previousTime) / 1000.0; // Convert ms to seconds
    previousTime = currentTime;

    // Integrate gyroscope data to get angles
    angleX += gyroX * dt;
    angleY += gyroY * dt;
    angleZ += gyroZ * dt;

    // Smoothing (average with previous angles)
    angleX = (angleX + previousAngleX) / 2;
    angleY = (angleY + previousAngleY) / 2;
    angleZ = (angleZ + previousAngleZ) / 2;

    // Remember the values for the next loop
    previousAngleX = angleX;
    previousAngleY = angleY;
    previousAngleZ = angleZ;

    // Let's project the cube's edges!
    for (int i = 0; i < 12; i++)
    {
        // Get the start and end vertices
        float *v1 = cube[edges[i][0]];
        float *v2 = cube[edges[i][1]];

        // Rotate and project the vertices to 2D
        int x1, y1, x2, y2;

        project(v1, angleX, angleY, angleZ, &x1, &y1);
        project(v2, angleX, angleY, angleZ, &x2, &y2);

        // Draw the edge
        // Draw three lines, offset by one pixel each, to get a thick line effect
        inkplate.drawLine(x1, y1, x2, y2, BLACK);
        inkplate.drawLine(x1 + 1, y1 + 1, x2 + 1, y2 + 1, BLACK);
        inkplate.drawLine(x1 + 2, y1 + 2, x2 + 2, y2 + 2, BLACK);
    }

    // Do partial (fast) update!
    inkplate.partialUpdate(true);

    // Wait 1ms so the frame rate isn't too fast
    delay(1);
}

// This function projects 3D space onto 2D with a set rotation
void project(float *v, float angleX, float angleY, float angleZ, int *x, int *y)
{
    // Rotate the vertex around the X axis
    float xr = v[0];
    float yr = v[1] * cos(angleX) - v[2] * sin(angleX);
    float zr = v[1] * sin(angleX) + v[2] * cos(angleX);

    // Rotate the vertex around the Y axis
    float xrr = xr * cos(angleY) + zr * sin(angleY);
    float yrr = yr;
    float zrr = -xr * sin(angleY) + zr * cos(angleY);

    // Rotate the vertex around the Z axis
    float xrrr = xrr * cos(angleZ) - yrr * sin(angleZ);
    float yrrr = xrr * sin(angleZ) + yrr * cos(angleZ);
    float zrrr = zrr;

    // Project the vertex to 2D
    float z = 4 / (4 + zrrr);
    *x = xrrr * z * 100 + inkplate.width() / 2;
    *y = yrrr * z * 100 + inkplate.height() / 2;
}