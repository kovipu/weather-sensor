#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SSID ""
#define PASSWORD ""
#define PIN D4
#define REPORT_INTERVAL 20 // in seconds
#define SERVER_ADDRESS "http://localhost/api/data"

int HighByte, LowByte, TempReading, SignBit, Tc_100;

StaticJsonBuffer<300> JSONbuffer;
JsonObject& JSONencoder = JSONbuffer.createObject();
HTTPClient http;

void setup() {
  Serial.begin(9600);

  // Configure the I/O
  pinMode(PIN, INPUT);
  digitalWrite(PIN, LOW);
  
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
  readTemp();

  delay(200);

  // JSON encode the data and print it nicely
  JSONencoder["temperature"] = Tc_100;
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

void resetPin() {
  digitalWrite(PIN, LOW);
  pinMode(PIN, OUTPUT);
  delayMicroseconds(500);
  pinMode(PIN, INPUT);
  delayMicroseconds(500);
}

// output byte b, bit at a time, least significant bit first
void outByte(byte b) {
  byte i;
  for (i=8; i > 0; i--) {
    digitalWrite(PIN, LOW);
    pinMode(PIN, OUTPUT);
    
    // test least significant bit
    if ((b & 0x01) == 1) {
      delayMicroseconds(5);
      pinMode(PIN, INPUT);
      delayMicroseconds(60);
    } else {
      delayMicroseconds(60);
      pinMode(PIN, INPUT);
    }

    // shift the bits around
    b = b >> 1;
  }
}

// read byte, least significant byte first
byte inByte() {
  byte i, current, total;
  total = 0;

  for (i=0; i<8; i++) {
    digitalWrite(PIN, LOW);
    pinMode(PIN, OUTPUT);
    delayMicroseconds(5);
    pinMode(PIN, INPUT);
    delayMicroseconds(5);
    current = digitalRead(PIN);
    delayMicroseconds(50);
    // shift the new bit in 
    total = (total >> 1) | (current << 7);
  }

  return(total);
}

void readTemp() {
  resetPin();
  outByte(0xCC);
  outByte(0x44);  // perform temperature conversion

  resetPin();
  outByte(0xCC);
  outByte(0xBE);

  LowByte = inByte();
  HighByte = inByte();

  TempReading = (HighByte << 8) + LowByte;
  SignBit = TempReading & 0x8000;

  // if negative, do Two's complement conversion
  if (SignBit) {
    TempReading = (TempReading ^ 0xFFFF) + 1;
  }

  // do conversion
  Tc_100 = (6 * TempReading) + (TempReading / 4);
}

