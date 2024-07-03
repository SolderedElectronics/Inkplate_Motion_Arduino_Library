/**
 **************************************************
 *
 * @file        Inkplate_6_MOTION_HTTP_GET_JSON.ino
 * @brief       This example will show you how to extract data from JSON via WiFi
 *              It will get weather data which will be printed on Inkplate
 *              Data is retrieved from api.open-meteo.com
 *
 *              To successfully run the sketch:
 *              -Install ArduinoJson library (https://github.com/bblanchon/ArduinoJson)
 *              -Enter WiFi data below
 *              -Edit GPS coordinates below, if you wish
 *              -Connect Inkplate 6 MOTION via USB-C cable
 *              -Press the programming button to put the device in the programming state
 *              -Upload the code
 *
 * @see         solde.red/333321
 *
 * @authors     Robert @ soldered.com
 * @date        July 2024
 ***************************************************/

// Include the Inkplate Motion Library
#include <InkplateMotion.h>

// Include ArduinoJSON library
#include <ArduinoJson.h>

// Include font
#include "fonts/ConsolaMono_Book8pt7b.h"

// Add WiFi data here
#define WIFI_SSID "Soldered"
#define WIFI_PASS "dasduino"

// Set your location data for the weather here
// By default, is set for Osijek, Croatia
#define WEATHER_LAT 45.5511
#define WEATHER_LON 18.6939

// How many minutes to wait between API calls and refreshing the screen
#define WAIT_MIN 5

// Variable to count the number of partial updates
int _partialUpdateCount = 0;

// Create an Inkplate Motion Object
Inkplate inkplate;

// Define struct which will hold weather data
struct currentWeatherData
{
    int weatherCode; // Weather code - to describe the weather
    float temp;      // Temperature
    float windSpeed; // Speed of the wind
    int windDir;     // Wind direction in degrees
} weatherData;

// Setup code, runs only once
void setup()
{
    // Initialize the Inkplate Motion Library in 1bit mode
    inkplate.begin(INKPLATE_1BW);

    // Clear the screen
    inkplate.display();

    // Set text printing option
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(3);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // Here's some technical information on how WiFi works on Inkplate 6 MOTION:
    // The onboard ESP32 is a co-processor, it's connected to the STM32 via SPI
    // It's running a firmware called ESP-AT: https://github.com/espressif/esp-at
    // This means, it will perform WiFi functions sent to it and communicate back to the STM32
    // This is all done seamlessly through the Inkplate Motion library

    // Initialize ESP32 WiFi
    if (!WiFi.init())
    {
        // If we're here, couldn't initialize WiFi, something is wrong!
        inkplate.println("ESP32-C3 initialization Failed! Code stopped.");
        inkplate.display();
        // Go to infinite loop after informing the user
        while (1)
        {
            delay(100);
        }
    }
    // Great, initialization was successful

    // Set mode to station
    if (!WiFi.setMode(INKPLATE_WIFI_MODE_STA))
    {
        inkplate.println("STA mode failed!");
        inkplate.partialUpdate(true);
        // Somehow this didn't work, go to infinite loop!
        while (1)
        {
            delay(100);
        }
    }

    // Connect to the WiFi network.
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.print("...");
    inkplate.partialUpdate(true);
    // This is the function which connects to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait until connected
    while (!WiFi.connected())
    {
        // Print a dot for each second we're waiting
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(1000);
    }
    // Great, we're connected! Inform the user
    inkplate.println("connected!");
    inkplate.partialUpdate(true);
    delay(1000); // Wait a bit

    // Clear the display
    inkplate.setCursor(0, 0);
    inkplate.clearDisplay();
    inkplate.display();
}

void loop()
{
    // Let's try getting the weather data
    if (!getWeatherData(&weatherData))
    {
        // Something went wrong! Inform the user and go to infinite loop
        inkplate.println("Getting data failed!");
        inkplate.display();
        while (true)
        {
            delay(100);
        }
    }
    else
    {
        // Great, data was retrieved successfully
        // Let's print the weather data
        inkplate.clearDisplay();
        inkplate.setTextColor(BLACK, WHITE);
        // Use a custom font
        inkplate.setFont(&ConsolaMono_Book8pt7b);
        inkplate.setTextSize(2);

        // Print current weather condiiton.
        inkplate.setCursor(22, 50);
        inkplate.println("Current weather: ");
        inkplate.setCursor(22, 80);
        inkplate.println(wmoCode(weatherData.weatherCode));
        inkplate.println();

        // Print current temperature.
        inkplate.setCursor(22, 250);
        inkplate.print("Temperature: ");
        inkplate.print(weatherData.temp, 1);
        inkplate.println("C");

        // Print current wind speed.
        inkplate.setCursor(22, 290);
        inkplate.print("Wind speed: ");
        inkplate.print(weatherData.windSpeed, 2);
        inkplate.println("km/s");

        // Print wind direciton.
        inkplate.setCursor(22, 330);
        inkplate.print("Wind direction: ");
        inkplate.print(weatherData.windDir, DEC);
        inkplate.println("deg");

        // Increment partial update counter.
        _partialUpdateCount++;

        if (_partialUpdateCount < 20)
        {
            // Do a partial update if the counter hasn't reached the limit
            inkplate.partialUpdate(false);
        }
        else
        {
            // Do a full refresh
            inkplate.display();

            // Reset the counter
            _partialUpdateCount = 0;
        }
    }

    // Wait for the set amount of time before getting data again
    delay(WAIT_MIN * 1000 * 60);
}

// Function to get the weather data and store it in the struct
bool getWeatherData(struct currentWeatherData *_currentDataPtr)
{
    // Char array for storing the URL which needs to get generated from our parameters
    char _url[260];
    // Buffer for the received JSON data
    char _jsonRaw[2000];

    // Create URL with LAT and LON
    // This URL contains all the options for the data which we're getting
    // For details, see https://open-meteo.com/en/docs
    sprintf(_url, "%s%d.%04d%s%d.%04d%s", "http://api.open-meteo.com/v1/forecast?latitude=", (int)WEATHER_LAT,
            abs((int)(WEATHER_LAT * 1000) % 1000), "&longitude=", (int)WEATHER_LON,
            abs((int)(WEATHER_LON * 1000) % 1000),
            "&current_weather=true&timezone=Europe%2FBerlin&current=temperature_2m,"
            "temperature,wind_speed_10m,wind_speed_10m");

    // Create WiFiClient object and specify that we want JSON as a response
    WiFiClient client;
    client.addHeader("Content-Type: application/json");

    // Try to connect to the client
    if (client.begin(_url))
    {
        // Great, we're connected!
        // Let's get data
        if (client.GET())
        {

            // Let's store JSON data in a buffer:

            // Number of bytes received
            int _chunks = 0;
            // Pointer to the buffer
            char *_ptr = _jsonRaw;
            // Clear buffer
            memset(_jsonRaw, 0, sizeof(_jsonRaw));

            // Get the data until there is no new data available
            while (client.available())
            {
                if (client.available())
                {
                    _chunks = client.read(_ptr, sizeof(_jsonRaw) - (_ptr - _jsonRaw));
                    _ptr += _chunks;
                }
            }

            // Add null-terminating char at the end
            *_ptr = '\0';

            // End client HTTP request
            client.end();

            // Now, parse the JSON:
            StaticJsonDocument<2000> doc;
            DeserializationError error = deserializeJson(doc, _jsonRaw);
            if (error)
            {
                // Error when parsing the JSON, notify the user
                inkplate.print("deserializeJson() failed: ");
                inkplate.println(error.c_str());
                inkplate.partialUpdate(true);
                return false; // Return that an error occurred
            }

            // Save everything into struct
            JsonObject current = doc["current_weather"];
            if (!current.isNull())
            {
                weatherData.weatherCode = current["weathercode"];
                weatherData.temp = current["temperature"];
                weatherData.windSpeed = current["windspeed"];
                weatherData.windDir = current["winddirection"];
            }
            else
            {
                inkplate.println("JSON does not contain 'current_weather'");
                return false;
            }

            // Everything is ok? Return true for success
            return true;
        }
    }

    // If something failed, return false.
    return false;
}

// Get string which describes the weather from the weathercode
char *wmoCode(int _code)
{
    switch (_code)
    {
    case 0:
        return "Clear sky";
        break;
    case 1:
    case 2:
    case 3:
        return "Mainly clear, partly cloudy, and overcast";
        break;
    case 45:
    case 48:
        return "Fog and depositing rime fog";
        break;
    case 51:
    case 53:
    case 55:
        return "Drizzle: Light, moderate, and dense intensity";
        break;
    case 56:
    case 57:
        return "Freezing Drizzle: Light and dense intensity";
        break;
    case 61:
    case 63:
    case 65:
        return "Rain: Slight, moderate and heavy intensity";
        break;
    case 66:
    case 67:
        return "Freezing Rain: Light and heavy intensity";
        break;
    case 71:
    case 73:
    case 75:
        return "Snow fall: Slight, moderate, and heavy intensity";
        break;
    case 77:
        return "Snow grains";
        break;
    case 80:
    case 81:
    case 82:
        return "Rain showers: Slight, moderate, and violent";
        break;
    case 85:
    case 86:
        return "Snow showers slight and heavy";
        break;
    case 95:
        return "Thunderstorm: Slight or moderate";
        break;
    case 96:
    case 99:
        return "Thunderstorm with slight and heavy hail";
        break;
    }

    return "Unknown";
}