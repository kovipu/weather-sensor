#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SSID "foo"
#define PASSWORD "********"
#define SENSOR_PIN D4
#define REPORT_INTERVAL 60 // in seconds
#define SERVER_ADDRESS "http://192.168.1.144:8001/api/data"

OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

StaticJsonBuffer<300> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();
HTTPClient http;

void setup() {
  Serial.begin(9600);
  
  // Connect to WiFi network
  Serial.println("Connecting to "SSID);

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {
  float temp;

  // attempt reading the temperature until a valid value is read
  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
  } while (temp == 85.0 || temp == (-127.0));
  
  Serial.print("Temperature: ");
  Serial.println(temp);

  // JSON encode the data and print it nicely
  JSONencoder["temperature"] = temp;
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);

  // POST the json to the server
  http.begin(SERVER_ADDRESS);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(JSONmessageBuffer);
  String payload = http.getString();

  Serial.println(httpCode);
  Serial.println(payload);

  http.end();
  
  delay(1000 * REPORT_INTERVAL);
}

