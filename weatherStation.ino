#include "weatherIcons.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "time.h"
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>
#include <JsonListener.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TOUCHBTN 14
#define BITMAP_HEIGHT   40
#define BITMAP_WIDTH    40

const char* ssid = "FunBox2-182";
const char* password = "#A2CD8H9Y*";
const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  //time zone for PL

const byte interruptPin = 14;     //pin to attach interrupt
volatile int numberOfScreen = 0;  //select screen to show
volatile int screenFlag = 0;
unsigned long startOfScreen = 0;

//initialize objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//DHT dht(DHTPIN, DHTTYPE);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
AsyncWebServer server(80);
WiFiClient client;

float tempAht = 0.0;
float humAht = 0.0;
float tempBMP280 = 0.0;
float pressBMP280 = 0.0;

String stateValue = "";

//Weather API data
String OPEN_WEATHER_MAP_APP_ID = "d7eba7eaf882d359fcc9fcdb8c5cec5a"; //Api key from OWM
String OPEN_WEATHER_MAP_LOCATION_ID = "3090764";          //Your City ID
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;
const uint8_t MAX_FORECASTS = 6;
OpenWeatherMapCurrent clientOWM;
OpenWeatherMapForecast clientOWMForecast;

//current weather stats
String currWeatherDescription = "";
float currWeatherTemp = 0.0;
int currWeatherHum = 0, currWeatherPressure = 0, currWeatherID = 0;

//forecast weather stats
String forecastWeatherDescription[MAX_FORECASTS]; //holds descripton of X-day weather
float forecastWeatherTempNightDay[MAX_FORECASTS]; //holds temps of X-day weather
int forecastWeatherHum[6], forecastWeatherID[6];  ////holds hum and ID of X-day weather
String weatherIcon = "";
String forecastWeatherIcon[6];

//Thingspeak
unsigned long myChannelNumber = 2037671;
String apiKey = "00F77JJ70NGRHW4C";
const char* serverTS = "api.thingspeak.com";

//timers
unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;
unsigned long previousMillis5 = 0;
const long intervalReadings = 5000;
const long intervalGetWeather = 300000;
const long intervalTimeUpdate = 1000;
const long intervalShowScreen = 5000;
const long intervalThingspeak = 60000;

String Date_str, Time_str, Day_str, Time_format; //variables for getting the time

int increment = 0;//screenFlag variable

void getReadings()  //get parameters from sensors
{
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  tempAht = temp.temperature;
  humAht = humidity.relative_humidity;
  tempBMP280 = bmp.readTemperature();
  pressBMP280 = bmp.readPressure() / 100;
  Serial.println(tempBMP280);
  Serial.println(humAht);
  if (numberOfScreen == 0) {
    oledDisplayCenter(String(tempBMP280, 1) + char(247) + "C", 0, 43, SCREEN_HEIGHT / 2, SCREEN_HEIGHT, 1);
    oledDisplayCenter(String(humAht, 0) + "%", 85, SCREEN_WIDTH, SCREEN_HEIGHT / 2, SCREEN_HEIGHT, 1);
  }
}

void getWeatherData() //connecting to weather server to get current weather
{
  OpenWeatherMapCurrentData data;
  clientOWM.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  clientOWM.setMetric(IS_METRIC);
  clientOWM.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  currWeatherID = data.weatherId;
  currWeatherTemp = data.temp;
  currWeatherPressure = data.pressure;
  currWeatherHum = data.humidity;
  currWeatherDescription = data.description.c_str();
}

void getForecastData() //connecting to weather server to get weather forecast
{
  OpenWeatherMapForecastData dataForecast[MAX_FORECASTS];
  clientOWMForecast.setMetric(IS_METRIC);
  clientOWMForecast.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {0, 12};
  clientOWMForecast.setAllowedHours(allowedHours, 2);
  uint8_t foundForecasts = clientOWMForecast.updateForecastsById(dataForecast, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  for (uint8_t i = 0; i < foundForecasts; i++) {
    forecastWeatherID[i] = dataForecast[i].weatherId;
    forecastWeatherTempNightDay[i] = dataForecast[i].temp;
  }
}

void UpdateLocalTime(String Format) {
  time_t now;
  time(&now);
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char hour_output[30], date_output[30], day_output[30];
  strftime(date_output, 30, "%d/%m", localtime(&now)); // Formats date as: 24/01
  strftime(hour_output, 30, "%T", localtime(&now));    // Formats time as: 14:05:49
  strftime(day_output, 30, "%a", localtime(&now));    //Formats day as abbreviated weekday name
  Date_str = date_output;
  Time_str = hour_output;
  Day_str = day_output;
}
/*
  Function to center given string and display it.
  Starting point in x axis is calculated as follows:
  x = minX + ((maxX - minX) - width) / 2, where mixX is left boundary, maxX is right boundary, width is width of the string (calculated by getTextBounds function)
  y = minY + ((maxY - minY) - fontSize * 8) / 2, where minY is upper boundary, maxY is lower boundary and height of the string is calculated based on its font size:
  height = fontSize * 8, default fontSize is 1, which corresponds to 8 pixels in height
*/
void oledDisplayCenter(String text, int minX, int maxX, int minY, int maxY, int fontSize) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  display.setTextSize(fontSize);
  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
  int x = minX + ((maxX - minX) - width) / 2;
  int y = minY + ((maxY - minY) - fontSize * 8) / 2;
  display.setCursor(x, y);
  // Print the text
  display.println(text);
  display.display();
}

/*
  Function assigns weather icon based on the weather ID that is read from OWM. Then the bitmap (they are 40x40 pixels) is centered between given coordinates.
  It is calculated by the formula, eg.
  minX + (maxX - minX - BITMAP_WIDTH) / 2
  0 + ((1/3 * 128) - 0 - 40)/2 = 0 + (42 - 40) / 2 = 1 --> x starting cooridante
*/
void assignWeatherIcon(int IDvalue, int minX, int maxX, int minY, int maxY) {
  if (IDvalue > 199 & IDvalue < 233)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, thunderstorm, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue > 299 & IDvalue < 323 || IDvalue > 519 & IDvalue < 532)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, showerRain, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue > 499 & IDvalue < 505)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, rain, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue == 511 || IDvalue > 599 & IDvalue < 623)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, snow, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue > 700 & IDvalue < 782)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, mist, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue == 800)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, clearSky, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue == 801)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, fewClouds, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue == 802)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, scatteredClouds, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
  if (IDvalue == 803 || IDvalue == 804)  display.drawBitmap(minX + (maxX - minX - BITMAP_WIDTH) / 2, minY + (maxY - minY - BITMAP_HEIGHT) / 2, brokenClouds, BITMAP_HEIGHT, BITMAP_WIDTH, WHITE);
}



ICACHE_RAM_ATTR void ISR () { //function to handle hardware interrupt
  numberOfScreen += 1;
  if (numberOfScreen > 2) {
    numberOfScreen = 0;
  }
  screenFlag = 0;
}

void setup() {
  Serial.begin(115200);
  //aht.begin();
  //bmp.begin();
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  if (!bmp.begin(0x77)) //I2C address of the sensor
  {
    Serial.println("Communication error with BMP280");
    while (1);
  }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("ESP8266 Web Server");
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  attachInterrupt(digitalPinToInterrupt(interruptPin), ISR, FALLING);

  //time configuration
  configTime(0, 0, "pl.pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  Time_format = "M";

  // Start server
  server.begin();
  Serial.println("HTTP server started");
  getWeatherData();
  getForecastData();
  Serial.println("Setup completed");
}

unsigned long prevTimer = 0;

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalReadings) {   //get sensor readings every X seconds
    previousMillis = currentMillis;
    Serial.println("getReadings");
    getReadings();
  }
  if (currentMillis - previousMillis2 >= intervalGetWeather) {  //get weather info every X minutes
    previousMillis2 = currentMillis;
    getWeatherData();
    getForecastData();
  }
  if (currentMillis - previousMillis3 >= intervalThingspeak) {
    if (client.connect(serverTS, 80)) // "184.106.153.149" or api.thingspeak.com
    {
      previousMillis3 = currentMillis;
      String postStr = apiKey;
      postStr += "&field1=";
      postStr += String(tempBMP280);
      postStr += "&field2=";
      postStr += String(pressBMP280);
      postStr += "&field3=";
      postStr += String(humAht);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
      Serial.println("Data sent");
      Serial.println();
    }
    else Serial.println("Client not connected");
    client.stop();
  }

  switch (numberOfScreen)
  {
    case 0: //main screen showing current humidity, pressure, temperature which are read from sensors
      if (!screenFlag) {
        display.clearDisplay();
        getReadings();
        screenFlag = 1;
      }
      if (currentMillis - previousMillis4 >= intervalTimeUpdate) {
        previousMillis4 = currentMillis;
        UpdateLocalTime(Time_format);
        display.setTextColor(WHITE, BLACK);
        oledDisplayCenter(Time_str, 0, SCREEN_WIDTH, 0, SCREEN_HEIGHT / 2, 2);
        oledDisplayCenter(Day_str, 43, 85, SCREEN_HEIGHT / 2, SCREEN_HEIGHT * 3 / 4, 2);
        oledDisplayCenter(Date_str, 43, 85, SCREEN_HEIGHT * 3 / 4, SCREEN_HEIGHT, 1);
      }
      break;

    case 1: //screen showing current parameters read from OWM
      if (!screenFlag)
      {
        startOfScreen = currentMillis;
        display.clearDisplay();
        display.setTextColor(WHITE, BLACK);
        oledDisplayCenter("Temp:", 0, SCREEN_WIDTH / 2, 0, 10, 1);
        oledDisplayCenter(String(currWeatherTemp, 1) + char(247) + "C", 0, SCREEN_WIDTH / 2, 10, 20, 1);
        oledDisplayCenter("Pressure:", 0, SCREEN_WIDTH / 2, 20, 30, 1);
        oledDisplayCenter(String(currWeatherPressure) + "hPa", 0, SCREEN_WIDTH / 2, 30, 40, 1);
        oledDisplayCenter("Humidity:", 0, SCREEN_WIDTH / 2, 40, 50, 1);
        Serial.println();
        Serial.print("currWeatherHum ");
        Serial.println(currWeatherHum);
        oledDisplayCenter(String(currWeatherHum) + "%", 0, SCREEN_WIDTH / 2, 50, 60, 1);
        assignWeatherIcon(currWeatherID, SCREEN_WIDTH / 2, SCREEN_WIDTH, 0, SCREEN_HEIGHT * 2 / 3);
        display.display();

        if (currWeatherDescription.indexOf(" ") == -1) {  //if description is longer than one word, divide it on blank sign
          oledDisplayCenter(currWeatherDescription, 64, 128, 40, 60, 1);
        }
        else  {
          oledDisplayCenter(currWeatherDescription.substring(0, currWeatherDescription.indexOf(" ")), 64, 128, 40, 50, 1);
          oledDisplayCenter(currWeatherDescription.substring(currWeatherDescription.indexOf(" ") + 1), 64, 128, 50, 60, 1);
        }

        screenFlag = 1;
      }
      if (currentMillis - startOfScreen >= intervalShowScreen) { //after X seconds return to main screen
        startOfScreen = 0;
        screenFlag = 0;
        numberOfScreen = 0;
      }
      break;

    case 2: //screen showing 3-day forecast from OWM
      if (!screenFlag)
      {
        startOfScreen = currentMillis;
        display.clearDisplay();
        display.setTextColor(WHITE, BLACK);

        /*
          Display temperatures for the next 3 days. Even indexes of MAX_FORECASTS are temperatures at night, odd are at day. The weather icon is assigned based on the weather during day.
          Every day corresponds to one third of the display width and icons are two thirds of the height. Then first temperature is for the day and the one below is for the night.
          Placement of these parameters is passed to the function oledDisplayCenter, which centers given String between passed x y boundaries.
        */
        for (int i = 0; i < MAX_FORECASTS; i++) {
          if (i % 2 == 0) {
            oledDisplayCenter(String(forecastWeatherTempNightDay[i], 1) + char(247) + "C" , SCREEN_WIDTH * i / 6, SCREEN_WIDTH * (i + 2) / 6, SCREEN_HEIGHT * 5 / 6, SCREEN_HEIGHT, 1);
          }
          else  {
            oledDisplayCenter(String(forecastWeatherTempNightDay[i], 1) + char(247) + "C" , SCREEN_WIDTH * (i - 1) / 6, SCREEN_WIDTH * (i + 1) / 6, SCREEN_HEIGHT * 2 / 3, SCREEN_HEIGHT * 5 / 6, 1);
            assignWeatherIcon(forecastWeatherID[i],                                         SCREEN_WIDTH * (i - 1) / 6, SCREEN_WIDTH * (i + 1) / 6, 0,                     SCREEN_HEIGHT * 2 / 3);
            display.display();
          }
        }
        screenFlag = 1;
      }
      if (currentMillis - startOfScreen >= intervalShowScreen) {
        startOfScreen = 0;
        screenFlag = 0;
        numberOfScreen = 0;
      }
      break;
  }
}
