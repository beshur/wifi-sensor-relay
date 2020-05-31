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

int sensor_ip[4] = {0, 0, 0, 0};
bool sensor_registered = false;

const int SENSOR_THRESHOLD = 20;
int last_measurement = null;

// HTTPClient http;

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleRegister() {
  if (server.method() != HTTP_POST) {
  server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = "POST form was:\n";

    if (server.arg("sensor_code") == SENSOR_ID) {
      sensor_registered = true;
      if (server.hasArg("sensor_ip")) {
//        sensor_ip = server.arg("sensor_ip");
      }
    }

    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
      if (server.argName(i) == "sensor_code" && server.arg(i) == SENSOR_ID) {
//        sensor_registered = true;
      }
    }

    Serial.println("handleRegister args:");
    Serial.println(message);
    Serial.println(sensor_registered);

    if (sensor_registered == true) {
      server.send(200, "text/html", "Registered");
    } else {
      server.send(401, "text/html", "Wrong arguments");
    }

  }
}



void setup() {
  delay(1000);
  Serial.begin(115200);
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
}
