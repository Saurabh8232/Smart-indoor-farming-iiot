#include <DHT.h>
#include <WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define R
#define G
#define B
DHT dht(5, DHTT11);
Adafruit_SGP30 sgp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,OLED_RESET);
const char* ssid = "OnePlus";
const char* password = "";
const char* mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char* token = "jEDGxQyHNYQrViRJbCbj";
float temperature;
float humidity;
int co2;
int tvoc;
float AQI;
String air_condition;

WiFiClient espClient;
PubSubClient client(espClient);
void RGB (int r, int g, int b) {
 analogWrite(R,r);
 analogWrite(G,g);
 analogWrite(B,b);
}
void ConnectToNetwork() {
 WiFi.begin(ssid, password);
 Serial.print("Connecting to WiFi...");

 int attempts = 0;
 while (WiFi.status() != WL_CONNECTED && attempts < 16) {
 delay(1000);
 Serial.print(".");
 attempts++;
 }

 if (WiFi.status() == WL_CONNECTED) {
 Serial.println("\nConnected to WiFi.");
 } else {
 Serial.println("\nFailed to connect to WiFi.");
 }
}
void AirCondition(){
 if (0 <= AQI && 50 >= AQI) {
 air_condition = "Good";
 RGB(0,255,0);
 }
 else if (51 <= AQI && 100 >= AQI) {
 air_condition = "Moderate";
 RGB(255,255,0);
 }
 else if (101 <= AQI && 300 >= AQI) {
 air_condition = "Bad";
 RGB(255,165,0);
 }
 else if (301 <= AQI && 400 >= AQI) {
 air_condition = "Very Bad";
 RGB(255,0,0);
 }
 else if (401 < AQI) {
 air_condition = "Hazardous";
 for (int i = 0; i < 5; i++) {
 setColor(255, 0, 0);
 delay(400);
 setColor(0, 0, 0);
 delay(400);
 }
 }
}
void ConnectToServer() {
 while (!client.connected()) {
 Serial.print("Connecting to ThingsBoard...");
 if (client.connect("ESP32_Client", token, "")) {
 Serial.println("Connected to ThingsBoard");
 } else {
 Serial.print("Failed, rc=");
 Serial.print(client.state());
 delay(2000);
 }
 }
}
void DataSending() {
 StaticJsonDocument<200> doc;
 doc["Temperature"] = temperature;
 doc["Humidity"] = humidity;
 doc["eCO2"] = co2;
 doc["TVOC"] = tvoc;
 char buffer[256];
 serializeJson(doc, buffer);
 client.publish("v1/devices/me/telemetry", buffer);
}
void displayData() {
 display.clearDisplay();
 display.setCursor(0, 0);
 display.print("Temp: "); display.print(temperature); display.println(" C");
 display.print("Humidity: "); display.print(humidity); display.println(" %");
 display.print("AQI: "); display.print(AQI); display.print("(");
 ;display.print(air_condition);display.print(")");
 display.display();

 Serial.print("Temp: "); Serial.print(temperature); Serial.println(" C");
 Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
 Serial.print("eCO2: "); Serial.print(co2); Serial.println(" ppm");
 Serial.print("TVOC: "); Serial.print(tvoc); Serial.println(" ppb");
 Serial.print("AQI: "); Serial.print(AQI);
}
void setup() {
 Serial.begin(115200);
 pinMode(R,OUTPUT);
 pinMode(G,OUTPUT);
 pinMode(B,OUTPUT);
 dht.begin();
 if (!display.begin(SSD1306_PAGEADDR, OLED_RESET)) {
 Serial.println(F("SSD1306 allocation failed"));
 for (;;)
 ;
 }
 display.setTextSize(1);
 display.setTextColor(SSD1306_WHITE);
 display.clearDisplay();
 if (!sgp.begin()) {
 Serial.println("SGP30 not found !");
 while (1);
 }
 ConnectToNetwork();
 client.setServer(mqttServer, mqttPort);
 ConnectToServer();
}
void loop() {
 if (!client.connected()) {
 ConnectToServer();
 }
 client.loop();
 if (isnan(temperature) || isnan(humidity)) {
 Serial.println("Failed to read from DHT sensor!");
 return;
 }
 temperature = dht.readTemperature();
 humidity = dht.readHumidity();
 if (!sgp.IAQmeasure()) {
 Serial.println("SGP30 measurement failed");
return;
 }
 co2 = sgp.eCO2;
 tvoc = sgp.TVOC;
 AQI = (co2*33.3)+(tvoc*.1);
 AirCondition();
 displayData();
 DataSending();
 delay(2000);
}