/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_Accelerometer_Cube.ino
 * @brief       This example will project a 3D cube with data from the accelerometer/gyroscope
 *
 * For info on how to quickly get started with Inkplate 6MOTION visit docs.inkplate.com
 *
 * @authors     Borna Biro and Robert Soric for soldered.com
 * @date        January 2025
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
#define ANGLE_MODIFIER 0.008

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

    // Turn on the lsm6dso32 peripheral
    inkplate.peripheralState(INKPLATE_PERIPHERAL_LSM6DSO32, true);
    delay(1000); // Wait a bit

    // Try to init the acccelerometer
    if (!inkplate.lsm6dso32.begin_I2C())
    {
        // Print message to user and show it on Inkplate
        inkplate.println("Couldn't init LSM6DSO32!");
        inkplate.display();
        while (true)
            ;
    }

    // Gyroscope init successful!
    inkplate.println("Init LSM6DSO32 OK!");
    inkplate.display();
    delay(1000); // Wait a bit so the user can see the message

    // Let's configure it, and print config data to Inkplate
    // This is useful to see all the available options
    inkplate.lsm6dso32.setAccelRange(LSM6DSO32_ACCEL_RANGE_16_G);
    inkplate.print("Accelerometer range set to: ");
    switch (inkplate.lsm6dso32.getAccelRange())
    {
    case LSM6DSO32_ACCEL_RANGE_4_G:
        inkplate.println("+-4G");
        break;
    case LSM6DSO32_ACCEL_RANGE_8_G:
        inkplate.println("+-8G");
        break;
    case LSM6DSO32_ACCEL_RANGE_16_G:
        inkplate.println("+-16G");
        break;
    case LSM6DSO32_ACCEL_RANGE_32_G:
        inkplate.println("+-32G");
        break;
    }

    inkplate.lsm6dso32.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
    inkplate.print("Gyro range set to: ");
    switch (inkplate.lsm6dso32.getGyroRange())
    {
    case LSM6DS_GYRO_RANGE_125_DPS:
        inkplate.println("125 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_250_DPS:
        inkplate.println("250 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_500_DPS:
        inkplate.println("500 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_1000_DPS:
        inkplate.println("1000 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_2000_DPS:
        inkplate.println("2000 degrees/s");
        break;
    }

    inkplate.lsm6dso32.setAccelDataRate(LSM6DS_RATE_104_HZ);
    inkplate.print("Accelerometer data rate set to: ");
    switch (inkplate.lsm6dso32.getAccelDataRate())
    {
    case LSM6DS_RATE_SHUTDOWN:
        inkplate.println("0 Hz");
        break;
    case LSM6DS_RATE_12_5_HZ:
        inkplate.println("12.5 Hz");
        break;
    case LSM6DS_RATE_26_HZ:
        inkplate.println("26 Hz");
        break;
    case LSM6DS_RATE_52_HZ:
        inkplate.println("52 Hz");
        break;
    case LSM6DS_RATE_104_HZ:
        inkplate.println("104 Hz");
        break;
    case LSM6DS_RATE_208_HZ:
        inkplate.println("208 Hz");
        break;
    case LSM6DS_RATE_416_HZ:
        inkplate.println("416 Hz");
        break;
    case LSM6DS_RATE_833_HZ:
        inkplate.println("833 Hz");
        break;
    case LSM6DS_RATE_1_66K_HZ:
        inkplate.println("1.66 KHz");
        break;
    case LSM6DS_RATE_3_33K_HZ:
        inkplate.println("3.33 KHz");
        break;
    case LSM6DS_RATE_6_66K_HZ:
        inkplate.println("6.66 KHz");
        break;
    }

    // Show the config and wait so the user can see it
    inkplate.display();
    delay(3000);
    inkplate.clearDisplay(); // Clear what was previously on the screen

    // Set 75 frames of partial updates for a full update
    inkplate.setFullUpdateTreshold(75);

    // Start measuring time
    previousTime = millis();
}

void loop()
{
    // First, clear what was previously in the frame buffer
    inkplate.clearDisplay();

    // Make sensor reading
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    inkplate.lsm6dso32.getEvent(&accel, &gyro, &temp);

    // Read values from the accelerometer (for display purposes)
    float accelX = accel.acceleration.x;
    float accelY = accel.acceleration.y;
    float accelZ = accel.acceleration.z;

    // Read values from the gyroscope
    float gyroX = gyro.gyro.x;
    float gyroY = gyro.gyro.y;
    float gyroZ = gyro.gyro.z;

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