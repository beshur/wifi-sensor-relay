#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include "sensor_config.h"


#ifndef APSSID
#define BLYNK_AUTH_CODE "aaaa"
#define APSSID "ESP-Pump-Server"
#define APPSK "1234567890"
#define SENSOR_ID "12345"
#endif

const String SERVER_HOST = "http://192.168.4.1:80";

ESP8266WebServer server(80);

HTTPClient http;

float range;
float response;

int trigPin = D4;
int echoPin = D3;

const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

const char* ssid = APSSID;
const char* password = APPSK;

int ledValue = HIGH;
long ledTimer;
const int LED_DELAY = 200;
long currentTime;
String currentHost = "";

void setupWiFi() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  currentHost = "http://" + WiFi.localIP().toString();
  // Print the IP address
  Serial.print("Use this URL: ");
  Serial.println(currentHost);

  delay(100);
  registerSensor();
}

String generateRegisterData() {
  String result = "sensor_code=" + String(SENSOR_ID) + "&sensor_host=" + currentHost;
  return result;
}

void registerSensor() {
  String URL = SERVER_HOST + "/register";
  String postData = generateRegisterData();
  Serial.println("registerSensor @ " + URL + "  with: " + postData);
  
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Content-Length", String(postData.length()));
  auto httpCode = http.POST(postData);
  Serial.print("register httpCode: ");
  Serial.println(httpCode);
  String payload = http.getString();
  Serial.println("Payload: " + payload);
  http.end();
}

void measure() {
  response = 0.0;
  digitalWrite(trigPin, LOW);   // ultrasonic launching low voltage at 2μs
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);  // ultrasonic launching high voltage at 10μs，at least at10μs
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);    // keeping ultrasonic launching low voltage
  response = pulseIn(echoPin, HIGH);  // time of error reading
  range = response / 5.8 / 10;  // converting time into distance（unit：cm）

  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = range;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
}

void checkLedStatus() {
  if (currentTime - ledTimer > LED_DELAY) {
    ledValue = HIGH;
  }
  digitalWrite(LED_BUILTIN, ledValue);
}

void toggleLed() {
  ledValue = LOW;
  ledTimer = millis();
}


void handleRoot() {
  server.send(200, "text/html", "ESP-Pump-Sensor");
}

void handleSensor() {
  server.send(200, "text/html", String(average));
//  toggleLed();
  Serial.println("/sensor Client disconnected\n");
}

// my board did not change its status or IP when lost Wi-Fi, unless this event handler
// see https://github.com/esp8266/Arduino/issues/5912#issuecomment-476073723
void onStationModeDisconnectedEvent(const WiFiEventStationModeDisconnected& evt) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  } else {
    Serial.println("WiFi disconnected...");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  ledTimer = millis();
  currentTime = millis();

  for (byte thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  Serial.begin(115200);

  // Start the server
  server.on("/", handleRoot);
  server.on("/sensor", handleSensor);
  server.begin();
  Serial.println("Server started");

  delay(100);
  setupWiFi();
}

void loop() {
  currentTime = millis();
  server.handleClient();
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.print("Reconnecting WiFi");
////    setupWiFi();
//    delay(100);
//  }
//  
  measure();
//  Serial.print("WiFi status connected: ");
//  Serial.print(WiFi.status() != WL_CONNECTED);
//  Serial.print("; IP: " + WiFi.localIP().toString());
//  Serial.print("; Sensor: ");
//  Serial.println(average);
  
//  checkLedStatus();
  
//  delay(100);
}
