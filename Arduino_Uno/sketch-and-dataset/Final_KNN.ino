#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <Arduino_KNN.h>

// ===================== Pin definitions =====================
#define SOIL_PIN A0
#define PUMP_PIN 3
#define DHT_PIN 2
#define DHT_TYPE DHT22

// ===================== Relay configuration =====================
#define RELAY_ACTIVE_LOW true  // true if relay turns ON when pin LOW

// ===================== Pump cooldown =====================
const unsigned long PUMP_COOLDOWN = 5UL * 60 * 1000; // 5 minutes

// ===================== WiFi & Firebase =====================
const char WIFI_SSID[] = "YourWiFiName";
const char WIFI_PASS[] = "YourWiFiPassword";

const char FIREBASE_HOST[] = "mandiligma-16e1c-default-rtdb.asia-southeast1.firebasedatabase.app";
const char FIREBASE_PATH[] = "/readings.json";
const char DATABASE_SECRET[] = "FfG5XWSYCZ0LUQbF8NSuvuhFZY4tGHVlPU2Alw6x";

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, FIREBASE_HOST, 443);

// ===================== Dataset =====================
const int NUM_SAMPLES = 24;
const int NUM_FEATURES = 3;

float dataset[NUM_SAMPLES][NUM_FEATURES] = {
  {0.10, 0.80, 0.30}, {0.20, 0.70, 0.40}, {0.25, 0.85, 0.35},
  {0.15, 0.75, 0.20}, {0.30, 0.90, 0.25}, {0.35, 0.78, 0.40},
  {0.75, 0.85, 0.50}, {0.80, 0.90, 0.60}, {0.70, 0.88, 0.55},
  {0.85, 0.95, 0.65}, {0.90, 0.92, 0.70}, {0.95, 0.89, 0.75},
  {0.25, 0.20, 0.40}, {0.30, 0.25, 0.35}, {0.35, 0.15, 0.30},
  {0.20, 0.10, 0.45}, {0.15, 0.05, 0.40}, {0.10, 0.15, 0.50},
  {0.85, 0.10, 0.70}, {0.90, 0.15, 0.65}, {0.95, 0.20, 0.60},
  {0.80, 0.25, 0.75}, {0.75, 0.18, 0.80}, {0.88, 0.12, 0.78}
};

int labels[NUM_SAMPLES] = {
  1,1,1,1,1,1,
  0,0,0,0,0,0,
  1,1,1,1,1,1,
  0,0,0,0,0,0
};

const int K = 5;
const float mins[NUM_FEATURES] = {0.0, 0.0, 0.0};
const float maxs[NUM_FEATURES] = {100.0, 50.0, 100.0};

// ===================== Globals =====================
DHT dht(DHT_PIN, DHT_TYPE);
KNNClassifier knn(NUM_FEATURES);
unsigned long lastPumpTime = 0;

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);

  // Wait until the Serial Monitor is ready (UNO R4 specific fix)
  delay(3000);  // give user time to open Serial Monitor
  Serial.println("Initializing...");

  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);

  // Load dataset
  for (int i = 0; i < NUM_SAMPLES; i++) {
    knn.addExample(dataset[i], labels[i]);
  }

  // ===================== WiFi =====================
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) { // 30 × 500ms = 15s
    Serial.print(".");
    delay(500);
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n Failed to connect to WiFi. Check SSID/password.");
  }

  Serial.println("System Ready!");
}

// ===================== LOOP =====================
void loop() {
  float moisture, temp, hum;
  if (!readSensors(&moisture, &temp, &hum)) {
    Serial.println("Sensor read failed, retrying...");
    delay(3000);
    return;
  }

  float new_point[NUM_FEATURES];
  normalizeInputs(moisture, temp, hum, new_point);

  int prediction = knn.classify(new_point, K);
  Serial.print("Predicted: ");
  Serial.println(prediction ? "ON (dry)" : "OFF (wet)");

  controlPump(prediction);

  sendToFirebase(moisture, temp, hum, prediction);

  Serial.println("-----------------------------------");
  delay(5000);
}

// ===================== SENSOR READ =====================
bool readSensors(float* moisture, float* temp, float* hum) {
  int raw_moisture = analogRead(SOIL_PIN);
  Serial.print("Raw moisture: "); Serial.println(raw_moisture);

  *moisture = 100.0 - (raw_moisture / 1023.0 * 100.0);
  *temp = dht.readTemperature();
  *hum = dht.readHumidity();

  if (isnan(*temp) || isnan(*hum)) {
    *temp = 25.0;
    *hum = 60.0;
  }

  Serial.print("Moisture(%): "); Serial.print(*moisture);
  Serial.print(" | Temp(°C): "); Serial.print(*temp);
  Serial.print(" | Hum(%): "); Serial.println(*hum);
  return true;
}

// ===================== NORMALIZATION =====================
void normalizeInputs(float moisture, float air_temp, float air_hum, float* new_point) {
  new_point[0] = constrain((moisture - mins[0]) / (maxs[0] - mins[0]), 0, 1);
  new_point[1] = constrain((air_temp - mins[1]) / (maxs[1] - mins[1]), 0, 1);
  new_point[2] = constrain((air_hum - mins[2]) / (maxs[2] - mins[2]), 0, 1);
  Serial.print("Normalized: ");
  Serial.print(new_point[0]); Serial.print(", ");
  Serial.print(new_point[1]); Serial.print(", ");
  Serial.println(new_point[2]);
}

// ===================== PUMP CONTROL =====================
void controlPump(int prediction) {
  if (prediction == 1 && (millis() - lastPumpTime >= PUMP_COOLDOWN)) {
    Serial.println("Pump ON...");
    digitalWrite(PUMP_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
    delay(6000);
    digitalWrite(PUMP_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    Serial.println("Pump OFF (6s)");
    lastPumpTime = millis();
  } else if (prediction == 0) {
    digitalWrite(PUMP_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  }
}

// ===================== FIREBASE UPLOAD =====================
void sendToFirebase(float moisture, float temp, float hum, int prediction) {
  String json = "{";
  json += "\"moisture\":" + String(moisture, 2) + ",";
  json += "\"temperature\":" + String(temp, 2) + ",";
  json += "\"humidity\":" + String(hum, 2) + ",";
  json += "\"prediction\":" + String(prediction) + ",";
  json += "\"timestamp\":" + String(millis());
  json += "}";

  Serial.println("Sending to Firebase...");

  client.beginRequest();
  client.post(String(FIREBASE_PATH) + "?auth=" + DATABASE_SECRET);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", json.length());
  client.beginBody();
  client.print(json);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Firebase response code: ");
  Serial.println(statusCode);
  if (statusCode > 0) {
    Serial.println("Response body: " + response);
  } else {
    Serial.println("Connection failed - check WiFi, host, or secret.");
  }
}