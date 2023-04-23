#include "ArduinoJson.h"
#include "DateTimeTZ.h"
#include "DHT.h"
#include "ESPDateTime.h"
#include "PubSubClient.h"
#include "WiFi.h"

#include "config.h"

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

void setupWifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi connected!");
  Serial.print("IP address is: ");
  Serial.println(WiFi.localIP());
}

void setupDateTime() {
  Serial.println("Setting up DateTime...");

  DateTime.setTimeZone(TZ_America_Sao_Paulo);
  DateTime.begin(20000);

  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  } else {
    Serial.printf("Date Now is %s\n", DateTime.toISOString().c_str());
    Serial.printf("Timestamp is %ld\n", DateTime.now());
  }
}

void reconnectMqtt() {
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection... ");

    if (pubSubClient.connect(CLIENT_ID)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" will retry in 5 seconds...");

      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting program...");

  setupWifi();
  setupDateTime();

  pubSubClient.setServer(MQTT_SERVER, MQTT_PORT);

  dht.begin();
}

void loop() {
  if (!pubSubClient.connected()) {
    reconnectMqtt();
  }

  pubSubClient.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(false);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C | Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  DynamicJsonDocument payload(1024);

  payload["temperature"] = temperature;
  payload["humidity"] = humidity;
  payload["clientId"] = CLIENT_ID;
  payload["measuredAt"] = DateTime.toISOString();

  Serial.println("Publishing message...");

  pubSubClient.publish(MQTT_PUBLISH_TOPIC, payload.as<String>().c_str());

  Serial.print("Message published! Next measurement in ");
  Serial.print(MEASUREMENT_INTERVAL_MS / 60000);
  Serial.println(" minutes...");

  delay(MEASUREMENT_INTERVAL_MS);
}
