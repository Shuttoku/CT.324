#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* telemetry_ssid = "KBU-WiFi";
const char* telemetry_password = "";
String loginURL = "http://10.29.0.1/login?username=kbuiot&password=kbuiot";

const char* command_1 = "sensor";  // Updated topic name
const char* command_2 = "";  // Updated topic name

#define mqtt_server "10.22.5.149"
#define mqtt_port 1883
#define mqtt_user "ops"
#define mqtt_password "ops2022"
#define mqtt_client_id "espV1"

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000;

const char* mqtt_topic_armtest = "/Sensor_topic";
#define LED_BUILTIN 2
#define R_rate A0
#define mqtt_topic "/Led_topic"

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Preparing...");

  pinMode(LED_BUILTIN, OUTPUT);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Setup completed.");
}

void loop() {
  setup_wifi();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int val = analogRead(R_rate);
  SendDataToMqtt(val, command_1);

  int sensorValue1 = analogRead(A0);
  int lux = sensorValue1;

  Serial.println("Sensor Value (LDR): " + String(sensorValue1));
//  Serial.println("Lux: " + String(lux));

  if (millis() - lastPublish >= publishInterval) {
    SendDataToMqtt(lux, command_2);
    lastPublish = millis();
  }

  delay(500);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  int i = 0;
  String cmd;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  while (i < length) msg += (char)payload[i++];
  Serial.println(msg);
  StaticJsonBuffer<300> JSONBuffer;
  JsonObject& parsed = JSONBuffer.parseObject(msg);
  if (!parsed.success()) {
    Serial.println("Parsing failed");
    delay(1000);
    return;
  }
  String msg_mode = parsed["mode"];
  Serial.println("msg_mode = " + msg_mode);

  if (msg_mode == "ledon") {
    Serial.println("Turning on LED...");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  } else if (msg_mode == "ledoff") {
    Serial.println("Turning off LED...");
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void setup_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", telemetry_ssid);
    WiFi.begin(telemetry_ssid, telemetry_password);
    connectWiFi();
  }
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("- WiFi connected");
  Serial.print("- IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Trying to login to hotspot . . .");
  WiFiClient wfclient;
  HTTPClient http;
  if (http.begin(wfclient, loginURL)) {
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Serial.println("[HTTP] Hotspot login successfully...");
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect to hotspot\n");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("- Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("  * already connected to WiFi");
      subscribeToCommand();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void subscribeToCommand() {
  String topic = mqtt_topic;
  client.subscribe(topic.c_str());
  Serial.println("  * Subscribed on topic: " + topic);
}

void SendDataToMqtt(int value, const char* command) {
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder[command] = value;
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  client.publish(mqtt_topic_armtest, JSONmessageBuffer);
}
