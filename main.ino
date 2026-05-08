#include "BluetoothSerial.h"
#include "HX711.h"
#include <ArduinoJson.h>

// =======================
// PINS (UNCHANGED)
// =======================
#define HX711_DOUT 4
#define HX711_SCK  5

#define HX710B_DOUT 18
#define HX710B_SCK  19

#define HALL1_PIN 34
#define HALL2_PIN 35
#define HALL3_PIN 32
#define HALL4_PIN 33

#define MOTOR_PIN 25

#define CPR_DURATION 75000 // 2 minutes in milliseconds
#define PULSE_DURATION 15000  // 15 seconds


BluetoothSerial SerialBT;
HX711 scale;

// =======================
// SYSTEM STATE
// =======================
enum Mode { IDLE, COURSE_CPR, COURSE_PULSE, TEST };
Mode currentMode = IDLE;

bool sessionActive = false;
unsigned long sessionStart = 0;

// =======================
// TIMING
// =======================
unsigned long lastSend = 0;
#define SEND_INTERVAL 100

// =======================
// MOTOR
// =======================
int bpm = 60;
unsigned long lastBeat = 0;
bool motorOn = false;

// Callback to monitor Bluetooth connection events
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("--- [EVENT] Bluetooth Device Connected! ---");
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("--- [EVENT] Bluetooth Device Disconnected! ---");
    sessionActive = false; // Safety: stop session on disconnect
  }
}

void updateMotor() {
  if (!sessionActive || currentMode != COURSE_PULSE) {
    digitalWrite(MOTOR_PIN, LOW);
    return;
  }

  unsigned long now = millis();
  unsigned long interval = 60000 / bpm;

  if (!motorOn && (now - lastBeat >= interval)) {
    motorOn = true;
    lastBeat = now;
    digitalWrite(MOTOR_PIN, HIGH);
  }

  if (motorOn && (now - lastBeat >= 120)) {
    motorOn = false;
    digitalWrite(MOTOR_PIN, LOW);
  }
}

// =======================
// HX710B RAW READ
// =======================
long readHX710B() {
  long v = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX710B_SCK, HIGH);
    v <<= 1;
    digitalWrite(HX710B_SCK, LOW);
    if (digitalRead(HX710B_DOUT)) v++;
  }
  digitalWrite(HX710B_SCK, HIGH);
  digitalWrite(HX710B_SCK, LOW);
  if (v & 0x800000) v |= ~0xFFFFFF;
  return v;
}

// =======================
// SELF TEST
// =======================
void sendHealthReport() {
  StaticJsonDocument<256> doc;
  doc["type"] = "health_report";
  doc["load_cell"] = scale.is_ready();
  doc["hall1"] = true;
  doc["hall2"] = true;
  doc["hall3"] = true;
  doc["hall4"] = true;
  doc["pressure"] = true;

  // Log to Bluetooth
  serializeJson(doc, SerialBT);
  SerialBT.println();

  // Log to Serial Monitor
  Serial.print(">> SENDING HEALTH: ");
  serializeJson(doc, Serial);
  Serial.println();
}

// =======================
// HANDLE BT INPUT
// =======================
void handleBT() {
  if (!SerialBT.available()) return;

  String msg = SerialBT.readStringUntil('\n');
  
  // Log raw incoming string
  Serial.print("<< RECEIVED: ");
  Serial.println(msg);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
    return;
  }

  const char* type = doc["type"];
  StaticJsonDocument<128> res;

  if (strcmp(type, "course") == 0) {
    int cid = doc["course_id"];
    if (cid == 0) {
      currentMode = COURSE_CPR;
      Serial.println("Mode set to: COURSE_CPR");
    } 
    else if (cid == 1) {
      currentMode = COURSE_PULSE;
      bpm = doc["freq"] | 60;
      Serial.printf("Mode set to: COURSE_PULSE (%d BPM)\n", bpm);
    }

    res["status"] = "ok";
    serializeJson(res, SerialBT);
    SerialBT.println();
    
    // Log response
    Serial.print(">> RESPONDING: ");
    serializeJson(res, Serial);
    Serial.println();

    delay(3000);
    sessionActive = true;
    sessionStart = millis();
    Serial.println("Session Started.");
  }

  else if (strcmp(type, "test") == 0) {
    currentMode = TEST;
    bpm = doc["freq"] | 60;
    Serial.println("Mode set to: TEST");

    res["status"] = "ok";
    serializeJson(res, SerialBT);
    SerialBT.println();

    Serial.print(">> RESPONDING: ");
    serializeJson(res, Serial);
    Serial.println();

    delay(3000);
    sessionActive = true;
    sessionStart = millis();
    Serial.println("Test Session Started.");
  }
}

// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Manikin System Initializing ---");

  scale.begin(HX711_DOUT, HX711_SCK);

  pinMode(HX710B_DOUT, INPUT);
  pinMode(HX710B_SCK, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  // Register the callback before starting BT
  SerialBT.register_callback(btCallback);
  
  if (!SerialBT.begin("ManikinESP32")) {
    Serial.println("Bluetooth initialization failed!");
  } else {
    Serial.println("Bluetooth Started: 'ManikinESP32' is now discoverable.");
  }

  delay(1000);
  sendHealthReport();
}

// =======================
// LOOP
// =======================


void loop() {
  handleBT();

  // ---------- SESSION TIMER LOGIC ----------
  if (sessionActive) {
    unsigned long elapsed = millis() - sessionStart;

    bool cprTimeout = (currentMode == COURSE_CPR && elapsed >= CPR_DURATION);
    bool pulseTimeout = (currentMode == COURSE_PULSE && elapsed >= PULSE_DURATION);

    if (cprTimeout || pulseTimeout) {
      Serial.print("--- [INFO] Session Timeout: ");
      Serial.print(currentMode == COURSE_CPR ? "CPR (2m)" : "Pulse (15s)");
      Serial.println(" ended. ---");

      sessionActive = false;
      currentMode = IDLE;
      digitalWrite(MOTOR_PIN, LOW); // Ensure motor stops

      // Notify the app
      StaticJsonDocument<128> endDoc;
      endDoc["type"] = "session_end";
      endDoc["reason"] = "timeout";
      serializeJson(endDoc, SerialBT);
      SerialBT.println();
    }
  }

  // ---------- RAW SENSOR READ ----------
  long raw_weight = scale.read();
  long raw_pressure = readHX710B();

  int h1 = analogRead(HALL1_PIN);
  int h2 = analogRead(HALL2_PIN);
  int h3 = analogRead(HALL3_PIN);
  int h4 = analogRead(HALL4_PIN);

  // ---------- MOTOR ----------
  updateMotor();

  // ---------- SEND DATA ----------
  if (sessionActive && (millis() - lastSend > SEND_INTERVAL)) {
    lastSend = millis();
    StaticJsonDocument<256> doc;

    if (currentMode == COURSE_CPR) {
      doc["type"] = "course_data";
      doc["course_id"] = 0;
      doc["hall1"] = h1;
      doc["hall2"] = h2;
      doc["hall3"] = h3;
      doc["hall4"] = h4;
      doc["weight_raw"] = raw_weight;
      doc["pressure_raw"] = raw_pressure;
      doc["time_left_ms"] = CPR_DURATION - (millis() - sessionStart);
    }
    else if (currentMode == COURSE_PULSE) {
      doc["type"] = "pulse_data";
      doc["course_id"] = 1;
      doc["time_left_ms"] = PULSE_DURATION - (millis() - sessionStart);
      // You can add other pulse-specific data here if needed
    }
    else if (currentMode == TEST) {
      doc["type"] = "test_data";
      doc["hall1"] = h1;
      doc["hall2"] = h2;
      doc["hall3"] = h3;
      doc["hall4"] = h4;
      doc["weight_raw"] = raw_weight;
      doc["pressure_raw"] = raw_pressure;
      doc["time_left_ms"] = CPR_DURATION - (millis() - sessionStart);
    }

    // Send to Bluetooth
    serializeJson(doc, SerialBT);
    SerialBT.println();
    
    // Log to Serial Monitor
    Serial.print(">> DATA SENT: ");
    serializeJson(doc, Serial);
    Serial.println();
  }
}