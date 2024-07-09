// Add an Inkplate Motion Libray to the Sketch.
#include <InkplateMotion.h>

// Include ArduinoJSON library. Get it from here: https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// Include the font file. Source: https://www.dafont.com/consola-mono.font
#include <ConsolaMono_Book8pt7b.h>

// Change WiFi SSID and password here.
#define WIFI_SSID   "Soldered-testingPurposes"
#define WIFI_PASS   "Testing443"

// Set your locaiton data for the weather here.
// By default, is set for Osijek, Croatia.
#define WEATHER_LAT 45.5511
#define WEATHER_LON 18.6939

// Create an Inkplate Motion Object.
Inkplate inkplate;

// Variable keeps track on how many times display has been partially updated.
// Force full update on startup.
int partialUpdateCount = 20;

struct currentWeatherData
{
    int humidity;
    int weaherCode;
    int windDir;
    float precipitation;
    int cloudCover;
    float temp;
    float pressure;
    float windSpeed;
    float windGust;
    char timeAndDate[20];
}weatherData;

void setup()
{
    // Setup a Serial communication for debug at 115200 bauds.
    Serial.begin(115200);

    // Print an welcome message (to know if the Inkplate board and STM32 are alive).
    Serial.println("Inkplate Motion Code Started!");

    // Initialize the Inkplate Motion Library.
    inkplate.begin(INKPLATE_1BW);

    // Set the text.
    inkplate.setCursor(0, 0);
    inkplate.setTextSize(2);
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setTextWrap(true);

    // First enable the WiFi peripheral.
    inkplate.peripheralState(INKPLATE_PERIPHERAL_WIFI, true);

    // Initialize At Over SPI library.
    if (!WiFi.init())
    {
        inkplate.println("ESP32-C3 initializaiton Failed! Code stopped.");
        inkplate.partialUpdate(true);

        while (1)
        {
            delay(100);
        }
    }

    inkplate.println("ESP32 Initialization OK!");
    inkplate.partialUpdate(true);

    WiFi.setMode(INKPLATE_WIFI_MODE_STA);

    // Connect to the WiFi network.
    inkplate.print("Connecting to ");
    inkplate.print(WIFI_SSID);
    inkplate.partialUpdate(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.connected())
    {
        inkplate.print('.');
        inkplate.partialUpdate(true);
        delay(500);
    }
    inkplate.println("connected!");
    inkplate.println("Getting the weather data from open-meteo...");
    inkplate.partialUpdate(true);

    // Load font file.
    inkplate.setFont(&ConsolaMono_Book8pt7b);
}

void loop()
{
    // Get new weather data every 300 seconds or so.
    if (getTheData(&weatherData)) printWeather(&weatherData, &partialUpdateCount);
    delay(1000 * 300);
}

bool getTheData(struct currentWeatherData *_currentDataPtr)
{
    // Char array for the URL.
    char _url[260];

    // Buffer for the JSON.
    char _jsonRaw[2000];

    // Create URL with LAT and LON.
    sprintf(_url, "%s%d.%04d%s%d.%04d%s", "https://api.open-meteo.com/v1/forecast?latitude=", (int)WEATHER_LAT, abs((int)(WEATHER_LAT * 1000) % 1000), "&longitude=", (int)WEATHER_LON, abs((int)(WEATHER_LON * 1000) % 1000), "&current=temperature_2m,relative_humidity_2m,precipitation,weather_code,cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=Europe%2FBerlin");

    // Add a header to get the file size (it can be only available, if connection: close is used).
    WiFiClient client;
    client.addHeader("Content-Type: application/json");

    // Try to conect to the client.
    if (client.connect(_url))
    {
        // Number of bytes received.
        int _chunks = 0;

        // Pointer to the buffer.
        char *_ptr = _jsonRaw;

        // Get the data until there is no new data available.
        while (client.available())
        {
            if (client.available())
            {
                _chunks = client.read(_ptr, 2000);
                _ptr += _chunks;
            }
        }

        // Add nul-terminating char at the end.
        *_ptr = '\0';
    
        // End client.
        client.end();

        // Parse the JSON.
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, _jsonRaw, 2000);
        if (error)
        {
            inkplate.print("deserializeJson() failed: ");
            inkplate.println(error.c_str());
            inkplate.partialUpdate(false);
            return false;
        }

        // Save everything into struct.
        JsonObject current = doc["current"];
        const char* current_time = current["time"]; // "2024-05-28T01:15"
        _currentDataPtr->temp = current["temperature_2m"]; // 14.1
        _currentDataPtr->humidity = current["relative_humidity_2m"]; // 78
        _currentDataPtr->precipitation = current["precipitation"]; // 0
        _currentDataPtr->weaherCode = current["weather_code"]; // 3
        _currentDataPtr->cloudCover = current["cloud_cover"]; // 94
        _currentDataPtr->pressure = current["surface_pressure"]; // 1002.5
        _currentDataPtr->windSpeed = current["wind_speed_10m"]; // 4.4
        _currentDataPtr->windDir = current["wind_direction_10m"]; // 171
        _currentDataPtr->windGust = current["wind_gusts_10m"]; // 7.6
        strlcpy(_currentDataPtr->timeAndDate, current["time"] | "N/A", 20);

        // Everything is ok? Return true for success.
        return true;
    }

    // If something have failed, return false.
    return false;
}

void printWeather(struct currentWeatherData *_currentDataPtr, int *_partialUpdateCount)
{
    // Set GFX for printing weather data.
    inkplate.clearDisplay();
    inkplate.setTextColor(BLACK, WHITE);
    inkplate.setCursor(0, 22);

    // Print current weather condiiton.
    inkplate.println("Current weather: ");
    inkplate.println(wmoCode(_currentDataPtr->weaherCode));
    inkplate.println();

    // Print current temperature.
    inkplate.print("Temperature: ");
    inkplate.print(_currentDataPtr->temp, 1);
    inkplate.println("C");

    // Print current relative humidity.
    inkplate.print("Relative humidity: ");
    inkplate.print(_currentDataPtr->humidity, DEC);
    inkplate.println("%");

    // Print current atmospheric pressure.
    inkplate.print("Air pressure: ");
    inkplate.print(_currentDataPtr->pressure, 1);
    inkplate.println("hPa");

    // Print current precipitation.
    inkplate.print("Precipitation: ");
    inkplate.print(_currentDataPtr->precipitation, 2);
    inkplate.println("mm");

    // Print current wind speed.
    inkplate.print("Wind speed: ");
    inkplate.print(_currentDataPtr->windSpeed, 2);
    inkplate.println("km/s");

    // Print current wind gust speed.
    inkplate.print("Wind gust speed: ");
    inkplate.print(_currentDataPtr->windGust, 2);
    inkplate.println("km/h");

    // Print wind direciton.
    inkplate.print("Wind direction: ");
    inkplate.print(_currentDataPtr->windDir, DEC);
    inkplate.println("deg");

    // Print current cloud coverage.
    inkplate.print("Cloud coverage: ");
    inkplate.print(_currentDataPtr->cloudCover, DEC);
    inkplate.println("%");

    // Print out time and date of the last weather update.
    inkplate.print("Last weather update:");
    inkplate.print(_currentDataPtr->timeAndDate);

    // Increment partial update counter.
    (*_partialUpdateCount)++;

    if ((*_partialUpdateCount) < 20)
    {
        // Do a partial update.
        inkplate.partialUpdate(false);
    }
    else
    {
        // Do a full refresh.
        inkplate.display();

        // Reset the counter.
        (*_partialUpdateCount) = 0;
    }
}

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