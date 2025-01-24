#include "DHT.h"
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 42
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define RAIN_ANALOG A4
#define RAIN_DIGITAL 7

#define MOISTURE_ANALOG A2
#define MOISTURE_DIGITAL 5

#define FAN_RELAY_PIN 39 
#define VALVE_SERVO_PIN 4 

#define IR_SENSOR_PIN A0
#define LED_RED_PIN 48
#define LED_YELLOW_PIN 6
#define LED_GREEN_PIN 14

Servo valveServo;

const float TEMP_THRESHOLD = 30.0; 
const float HUMIDITY_THRESHOLD = 80.0; 

const char* ssid = "iPhone";  // Replace with your Wi-Fi SSID
const char* password = "afifcomey";  // Replace with your Wi-Fi password
const char* mqtt_server = "34.70.33.24";  // Replace with your MQTT broker address
const char* mqtt_topic = "cpcproject";
const int mqtt_port = 1883;
const int dht11pin = 42;
const int rainanalogpin = A4;
const int raindidigitalpin = 7;
const int moistureanalogpin = A2;
const int moisturedigitalpin = 5;
const int fanrelaypin = 39;
const int valveservopin = 4;
const int irsensorpin = A0;
const int ledredpin = 48;
const int ledyellowpin = 6;
const int ledgreeenpin = 14;
char buffer[128] = " ";

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("mushroom/control/fan");
      client.subscribe("mushroom/control/valve");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == "mushroom/control/fan") {
    if (message == "ON") {
      digitalWrite(FAN_RELAY_PIN, HIGH);
      Serial.println("Fan turned ON via MQTT.");
    } else if (message == "OFF") {
      digitalWrite(FAN_RELAY_PIN, LOW);
      Serial.println("Fan turned OFF via MQTT.");
    }
  }

  if (String(topic) == "mushroom/control/valve") {
    if (message == "OPEN") {
      valveServo.write(180);
      Serial.println("Valve Opened via MQTT.");
    } else if (message == "CLOSE") {
      valveServo.write(0);
      Serial.println("Valve Closed via MQTT.");
    }
  }
}

void setup() {
  Serial.begin(9600);

  dht.begin();
  Serial.println("DHT11 Sensor Initialized");

  pinMode(RAIN_DIGITAL, INPUT);
  Serial.println("Rain Sensor Initialized");

  pinMode(MOISTURE_DIGITAL, INPUT);
  Serial.println("Soil Moisture Sensor Initialized");

  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW); 
  Serial.println("Fan Relay Initialized");

  valveServo.attach(VALVE_SERVO_PIN);
  valveServo.write(0); 
  Serial.println("Valve Servo Motor Initialized");

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  Serial.println("IR Sensor and LEDs Initialized");

  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    client.publish("mushroom/sensor/humidity", String(humidity).c_str());
    client.publish("mushroom/sensor/temperature", String(temperature).c_str());
  }

  int rainDigitalValue = digitalRead(RAIN_DIGITAL);
  client.publish("mushroom/sensor/rain", String(rainDigitalValue).c_str());

  if (!isnan(temperature) && temperature > TEMP_THRESHOLD) {
    digitalWrite(FAN_RELAY_PIN, HIGH); 
    Serial.println("Fan ON: Temperature exceeded threshold.");
  } else if (rainDigitalValue == HIGH) { 
    digitalWrite(FAN_RELAY_PIN, LOW); 
    Serial.println("Fan OFF: Rain detected.");
  } else {
    digitalWrite(FAN_RELAY_PIN, LOW); 
    Serial.println("Fan OFF: Temperature is normal.");
  }

  int moistureDigitalValue = digitalRead(MOISTURE_DIGITAL);
  client.publish("mushroom/sensor/moisture", String(moistureDigitalValue).c_str());

  if (moistureDigitalValue == LOW) { 
    valveServo.write(180); 
    Serial.println("Valve Opened: Soil moisture is low.");
  } else if (rainDigitalValue == HIGH) { 
    valveServo.write(0); 
    Serial.println("Valve Closed: Rain detected.");
  } else if (humidity < HUMIDITY_THRESHOLD) { 
    valveServo.write(180); 
    Serial.println("Valve Opened: Humidity below threshold.");
  } else {
    valveServo.write(0); 
    Serial.println("Valve Closed: Conditions are normal.");
  }

  int irSensorValue = analogRead(IR_SENSOR_PIN);
  if (irSensorValue < 400) { 
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
    Serial.println("Mushroom ready to harvest.");
  } else if (irSensorValue >= 400 && irSensorValue < 700) { 
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_YELLOW_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
    Serial.println("Mushroom is growing.");
  } else { 
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_YELLOW_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);
    Serial.println("No mushroom detected.");
  }

  delay(2000);
}
