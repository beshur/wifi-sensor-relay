# Wi-Fi Sensor and Relay

This is the project to build a Wi-Fi connected sensor and a relay.

The purpose of this is to automate water pumping in the country house.

# Plan

Use two ESP32s or similar controllers.

One is going to be a client with a sensor (probably ultrasonic to measure water level in the tank).

The second one is going to be a server that will receive data from the sensor controller over Wi-Fi
and in turn control the relay where the pump will be connected.
