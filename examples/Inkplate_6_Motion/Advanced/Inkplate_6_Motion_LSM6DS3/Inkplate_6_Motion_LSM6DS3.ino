// Include Inkplate Motion Library.
#include <InkplateMotion.h>

// Create a Inkplate Motion object.
Inkplate inkplate;

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

// Setup code, runs only once
void setup()
{
    // Initialize serial communication at 115200 bauds.
    Serial.begin(115200);

    // Send debug message.
    Serial.println("Hello from Inkplate 6 Motion");

    // Init Inkplate library (you should call this function ONLY ONCE)
    inkplate.begin();

    // Set automatic full update after 200 parital updates.
    inkplate.setFullUpdateTreshold(200);

    // Set text size to be 2x larger than default (5x7px)
    inkplate.setTextSize(2);
    // Set the text color to black also.
    inkplate.setTextColor(BLACK);

    // Try to init the acccelerometer.
    inkplate.lsm6ds3.begin();
}

void loop()
{
    // First, clear what was previously in the frame buffer.
    inkplate.clearDisplay();

    // Read values from the accelerometer
    float accelX = inkplate.lsm6ds3.readRawAccelX();
    float accelY = inkplate.lsm6ds3.readRawAccelY();
    float accelZ = inkplate.lsm6ds3.readRawAccelZ();

    // Read values from the gyroscope
    float gyroX = inkplate.lsm6ds3.readFloatGyroX();
    float gyroY = inkplate.lsm6ds3.readFloatGyroY();
    float gyroZ = inkplate.lsm6ds3.readFloatGyroZ();

    // Print accelerometer readings on the display
    inkplate.setCursor(40, 630);
    inkplate.print("ACC X:");
    inkplate.print(accelX, 4);
    inkplate.setCursor(40, 650);
    inkplate.print("ACC Y:");
    inkplate.print(accelY, 4);
    inkplate.setCursor(40, 670);
    inkplate.print("ACC Z:");
    inkplate.print(accelZ, 4);

    // Print gyroscope readings on the display also
    inkplate.setCursor(40, 690);
    inkplate.print("GYRO X:");
    inkplate.print(gyroX, 4);
    inkplate.setCursor(40, 710);
    inkplate.print("GYRO Y:");
    inkplate.print(gyroY, 4);
    inkplate.setCursor(40, 730);
    inkplate.print("GYRO Z:");
    inkplate.print(gyroZ, 4);

    // Let's draw the cube!
    // Compute the angle modifier variables from the accelerometer data
    angleX = accelX * ANGLE_MODIFIER;
    angleY = accelY * ANGLE_MODIFIER;
    angleZ = accelZ * ANGLE_MODIFIER;

    // Calculate the average between the previous
    // This makes the movement smoother
    angleX = (angleX + previousAngleX) / 2;
    angleY = (angleY + previousAngleY) / 2;
    angleZ = (angleZ + previousAngleZ) / 2;

    // Remember the value for the next loop
    previousAngleX = angleX;
    previousAngleY = angleY;
    previousAngleZ = angleZ;

    // Let's project the cube's edges!
    // For each edge...
    for (int i = 0; i < 12; i++)
    {
        // Get the start and end vertices
        float *v1 = cube[edges[i][0]];
        float *v2 = cube[edges[i][1]];

        // Rotate and project the vertices to 2D
        int x1, y1, x2, y2;

        // Project it, notice that X, Y and Z are rearranged here and not in the default order
        // This is due to the orientation of the gyroscope on the actual board
        project(v1, angleY, angleZ, angleX, &x1, &y1);
        project(v2, angleY, angleZ, angleX, &x2, &y2);

        // Draw the edge
        inkplate.drawLine(x1, y1, x2, y2, BLACK);
    }

    // Finally, let's update the screen. Keep the epaper supply on all the time to speed up refresh.
    // Library will automatically do full update to clear the screen.
    inkplate.partialUpdate(true);

    // Wait 10ms so the frame rate isn't too fast
    delay(5);
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