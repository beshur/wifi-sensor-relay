/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include "server_config.h"

#ifndef APSSID
#define BLYNK_AUTH_CODE "aaaa"
#define APSSID "ESP-Pump-Server"
#define APPSK "1234567890"
#define SENSOR_ID "12345"
#endif

const int VERSION = 1;
// hw
const int RELAY_PIN = D4;

// software
HTTPClient http;

String sensor_host = "";

// thresholds in cm; sensor value
const int SENSOR_FULL_THRESHOLD = 20;
const int SENSOR_LOW_THRESHOLD = 160;
int last_measurement = -1;

// default check is once per 15 minutes
const int SENSOR_POLL_DELAY = 900000;
// while pump is active check every second
const int ACTIVE_SENSOR_POLL_DELAY = 1000;

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

int SERVER_STATE = 0;
const int STATE_IDLE = 0;
const int STATE_ERROR = 1;
const int STATE_SENSOR_CONNECTED = 2;
const int STATE_SENSOR_OK = 3;
const int STATE_SENSOR_LOW = 4;
const int STATE_RELAY_CLOSED = 5;

int get_state() {
  return SERVER_STATE;
}

ESP8266WebServer server(80);

void state_on_registered() {
  Serial.println("state_on_registered");
  SERVER_STATE = STATE_SENSOR_CONNECTED;
}
void state_on_sensor_full() {
  Serial.println("state_on_sensor_full");
  SERVER_STATE = STATE_SENSOR_OK;
}

void state_on_sensor_low() {
  Serial.println("state_on_sensor_low");
  SERVER_STATE = STATE_SENSOR_LOW;
}

void state_on_relay_close() {
  Serial.println("state_on_relay_close");
  SERVER_STATE = STATE_RELAY_CLOSED;
}

void state_on_error() {
  Serial.println("state_on_error");
  SERVER_STATE = STATE_ERROR;
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleRegister() {
  if (server.method() != HTTP_POST) {
  server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = "";

    if (server.arg("sensor_code") == SENSOR_ID) {
      Serial.println("sensor_registered true");
      if (server.hasArg("sensor_host")) {
        sensor_host = server.arg("sensor_host");
        state_on_registered();
        Serial.println("Sensor registered @ " + sensor_host);
      }
    }
    // debug
    Serial.println("handleRegister args:");
    Serial.println(server.arg("sensor_code"));
    Serial.println(SENSOR_ID);
    Serial.println(server.arg("sensor_host"));
    Serial.println(message);

    if (get_state() == STATE_SENSOR_CONNECTED) {
      server.send(200, "text/html", "Registered");
    } else {
      server.send(401, "text/html", "Wrong arguments");
    }

  }
}

void readSensorData() {
  HTTPClient http;
  String URL = sensor_host + "/sensor";
  http.begin(URL);
  
  Serial.println("readSensorData @ " + URL);
 
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("readSensorData GET: " + payload);
    check_measurement(payload.toInt());
  } else {
    state_on_error();
  }
 
  http.end();
  // #TODO replace with proper non-blocking delay
  delay(ACTIVE_SENSOR_POLL_DELAY);
}

void check_measurement(int tmp_last_measurement) {
  if (tmp_last_measurement >= 0) {
    last_measurement = tmp_last_measurement;
    // normal operation
    if (get_state() == STATE_RELAY_CLOSED) {
      // relay closed, waiting till the tank reaches FULL
      if (last_measurement <= SENSOR_FULL_THRESHOLD) {
        state_on_sensor_full();
      }
    } else {
      // relay open, just watching the tank
      if (last_measurement >= SENSOR_LOW_THRESHOLD) {
        state_on_sensor_low();
      }
    }
    
  } else {
    state_on_error();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  
  Serial.println();
  Serial.print("Configuring access point...");

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/register", handleRegister);
  server.begin();
  Serial.println("HTTP server started");

  WiFi.hostname("relay-server");
}

void loop() {
  server.handleClient();

  
/*
 * State Loop
 * 
 * If registered
   * If relay open then poll sensor with timer REGULAR
   * If SENSOR_LOW then close relay
     * If SENSOR_OK then open relay
     * poll sensor with timer ACTIVE
 * 
 */
  if (get_state() >= STATE_SENSOR_CONNECTED) {    
    readSensorData();

    if (get_state() == STATE_SENSOR_LOW) {
      state_on_relay_close();
    }
  }

  digitalWrite(RELAY_PIN, get_state() == STATE_RELAY_CLOSED);

}
