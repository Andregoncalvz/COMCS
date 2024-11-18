/************************************/
/*        ISEP MESCC COMCS          */
/*                                  */
/*  Group 5                         */
/*    -> André Gonçalves - 1210804  */
/*    -> Jorge Moreira - 1201458    */
/*                                  */
/************************************/

#include <WiFi.h>
#include <WiFiUdp.h>
#include "DHT.h"  // DHT sensor library


// WiFi and UDP Configuration
const char *ssid = "NOS-17A6";
const char *pwd = "WPSN47JG";
//const char *ssid = "MEO-CASA";
//const char *pwd = "229823683";
//const char *ssid = "IoTDEI2";
//const char *pwd = "#8tud3nt2024";
const char *udpAddress = "192.168.1.232";
const int udpPort = 9999;

WiFiUDP udp;

char response[300];

// DHT Sensor Configuration
#define DHTPIN 21
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// QoS Setting
const float qos = 1; // QoS level (0 or 1)
float lastTemp = 0;
float lastHumidity = 0;

void setup() {
  delay(2000);
  Serial.begin(9600);
  
  // Connect to Wi-Fi
  Serial.println("Connecting to the WiFi network...");
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize UDP
  udp.begin(udpPort);

  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  float currentHumidity = dht.readHumidity();
  float currentTemp = dht.readTemperature();
  
  if (isnan(currentHumidity) || isnan(currentTemp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print current readings
  Serial.printf("Humidity: %.2f%%, Temperature: %.2f°C\n", currentHumidity, currentTemp);

  // Save last values for next differential comparison
  lastTemp = currentTemp;
  lastHumidity = currentHumidity;

  // Send data over UDP with QoS level
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "QoS: %.1f, Temp: %.2f, Humidity: %.2f", qos, currentTemp, currentHumidity);
  udp.beginPacket(udpAddress, udpPort);
  udp.write((uint8_t *)buffer, strlen(buffer));
  udp.endPacket();
  

  // Wait for acknowledgment (server response) if QoS is 1
  if (qos == 1) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(response, sizeof(response));
      if (len > 0) {
        response[len] = '\0';  // Null-terminate the string
        Serial.printf("Received response: %s\n", response);  // Print the response from the server
      }
    }
  }

  delay(10000); // Wait before sending the next reading
}