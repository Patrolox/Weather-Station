# Weather Station Project

This project is a weather station application based on NodeMCU, which collects and displays weather data from various sensors. It utilizes Wi-Fi connectivity to retrieve current and future weather information from OpenWeatherMaps. In addition, it retrieves time and date from an internet clock. The project incorporates a BMP280 sensor for measuring indoor parameters, while an OLED screen serves as the display.

## Features

- Collects real-time weather data from OpenWeatherMaps API
- Retrieves time and date from an internet clock
- Measures indoor temperature and humidity using a BMP280 sensor
- Utilizes an OLED screen for displaying weather and sensor data
- Allows toggling between different windows using a touch button
- Stores sensor readings on Thingiverse for further analysis and visualization
- Provides a simple webpage that uses Thingiverse API to display charts and current sensor values

## UI Description

- The interface consists of three windows.
- Upon starting the weather station, the home window is displayed. This window shows the current time, date, temperature, and humidity in a room.
- By pressing the touch button, you can navigate to the current weather window. This window displays the current weather outside along with an image representing the current weather conditions.
- Continuing to press the touch button will take you to the weather forecast window. In this window, users can view the weather forecast for the next three days. The window displays both daytime and nighttime temperatures, along with weather images to represent the forecasted conditions.
- To provide an additional means of accessing and visualizing the stored data, the weather station offers a simple webpage. This webpage utilizes the Thingiverse API to retrieve and display charts and current sensor values. Users can access the webpage to view graphical representations of the sensor data.


