// ===== BLYNK CONFIG =====
#define BLYNK_TEMPLATE_ID "TMPL2pVDB7k7E"
#define BLYNK_TEMPLATE_NAME "caixadagua"
#define BLYNK_AUTH_TOKEN "YWp6DMhuwbJZ8WeiEYbGOrX8dUlcvsbj"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>

// ===== WIFI =====
char ssid[] = "BDAG";
char pass[] = "bdag2018";

// ===== MQTT =====
const char* mqtt_server = "10.64.95.83";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// ===== PINOS =====
const int trig1 = 14;
const int echo1 = 13;

const int trig2 = 22;
const int echo2 = 23;

const int relePin = 26;

// ===== CALIBRAÇÃO =====
const float minDistance = 3.28;
const float maxDistance = 13.43;

// ===== MQTT CONNECT =====
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("OK");
    } else {
      Serial.print("Erro: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ===== FUNÇÃO DISTÂNCIA =====
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) return maxDistance;

  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(trig1, OUTPUT);
  pinMode(echo1, INPUT);

  pinMode(trig2, OUTPUT);
  pinMode(echo2, INPUT);

  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH); // relé desligado

  // WiFi
  WiFi.begin(ssid, pass);
  Serial.print("WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("OK");

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  Blynk.run();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // ===== LEITURA =====
  float distance1 = getDistance(trig1, echo1);
  delay(300);

  float distance2 = getDistance(trig2, echo2);
  delay(300);

  // ===== LIMITAR =====
  distance1 = constrain(distance1, minDistance, maxDistance);
  distance2 = constrain(distance2, minDistance, maxDistance);

  // ===== CONVERTER (%) =====
  int level1 = (int)((maxDistance - distance1) * 100 / (maxDistance - minDistance));
  int level2 = (int)((maxDistance - distance2) * 100 / (maxDistance - minDistance));

  // ===== REDUNDÂNCIA (1 SENSOR ATIVO) =====
  int level1_out = 0;
  int level2_out = 0;

  float distance1_out = 0;
  float distance2_out = 0;

  bool sensor1_ok = (distance1 > minDistance && distance1 < maxDistance);

  if (sensor1_ok) {
    // usa sensor 1
    level1_out = level1;
    distance1_out = distance1;

    Serial.println("Usando Sensor 1");

  } else {
    // usa sensor 2
    level2_out = level2;
    distance2_out = distance2;

    Serial.println("Sensor 1 FALHOU → usando Sensor 2");
  }

  // ===== SERIAL =====
  Serial.println("------");

  Serial.print("Sensor 1: ");
  Serial.print(distance1_out);
  Serial.print(" cm | ");
  Serial.print(level1_out);
  Serial.println("%");

  Serial.print("Sensor 2: ");
  Serial.print(distance2_out);
  Serial.print(" cm | ");
  Serial.print(level2_out);
  Serial.println("%");

  // ===== BLYNK =====
  Blynk.virtualWrite(V0, level1_out);
  Blynk.virtualWrite(V1, level2_out);

  // ===== MQTT JSON =====
  String payload = "{";
  payload += "\"sensor1\":" + String(distance1_out, 2) + ",";
  payload += "\"sensor2\":" + String(distance2_out, 2) + ",";
  payload += "\"rele\":" + String(digitalRead(relePin) == LOW ? 1 : 0);
  payload += "}";

  client.publish("teste", payload.c_str());

  Serial.println("JSON enviado:");
  Serial.println(payload);

  delay(3000);
}

// ===== RELÉ VIA BLYNK =====
BLYNK_WRITE(V3) {
  int estado = param.asInt();

  digitalWrite(relePin, !estado);

  Serial.print("Rele: ");
  Serial.println(estado ? "LIGADO" : "DESLIGADO");
}