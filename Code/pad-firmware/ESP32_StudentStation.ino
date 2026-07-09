/*
  Classroom Mood Monitor — ESP32 Student Station
  ===============================================
  Change STUDENT_ID, WIFI_SSID, WIFI_PASS, BRIDGE_HOST for each board.

  Wiring (all buttons: one leg to pin, other leg to GND):
    GPIO 13 — Happy button
    GPIO 12 — Neutral button
    GPIO 14 — Sad button
    GPIO 27 — Question button
    GPIO 26 — Limit switch (activates station)
    GPIO 25 — Green LED (lights when active)
*/

#include <WiFi.h>
#include <ArduinoWebsockets.h>
using namespace websockets;

// ── Change these for each ESP32 ────────────────────────────────
const int   STUDENT_ID  = 1;
const char* WIFI_SSID   = "YOUR_WIFI_SSID";
const char* WIFI_PASS   = "YOUR_WIFI_PASSWORD";
const char* BRIDGE_HOST = "172.20.10.3";
const int   BRIDGE_PORT = 8765;
// ───────────────────────────────────────────────────────────────

const int PIN_HAPPY    = 13;
const int PIN_NEUTRAL  = 12;
const int PIN_SAD      = 14;
const int PIN_QUESTION = 27;
const int PIN_LIMIT    = 26;
const int PIN_LED      = 25;

bool stationActive = false;
bool lastHappy     = HIGH;
bool lastNeutral   = HIGH;
bool lastSad       = HIGH;
bool lastQuestion  = HIGH;
bool lastLimit     = HIGH;

WebsocketsClient wsClient;
bool wsConnected      = false;
unsigned long lastReconnect = 0;
const unsigned long RECONNECT_MS = 3000;

void connectWS() {
  Serial.println("[WS] Connecting to bridge...");
  wsConnected = wsClient.connect(BRIDGE_HOST, BRIDGE_PORT, "/");
  if (wsConnected) {
    Serial.println("[WS] Connected!");
    String hello = "{\"student\":" + String(STUDENT_ID) + ",\"type\":\"hello\"}";
    wsClient.send(hello);
  } else {
    Serial.println("[WS] Failed, will retry...");
  }
}

void sendVote(String mood) {
  if (!wsConnected) return;
  String msg = "{\"student\":" + String(STUDENT_ID) +
               ",\"type\":\"vote\",\"mood\":\"" + mood + "\"}";
  Serial.println("[TX] " + msg);
  wsClient.send(msg);
}

void sendQuestion() {
  if (!wsConnected) return;
  String msg = "{\"student\":" + String(STUDENT_ID) + ",\"type\":\"question\"}";
  Serial.println("[TX] " + msg);
  wsClient.send(msg);
}

void sendActive(bool active) {
  if (!wsConnected) return;
  String msg = "{\"student\":" + String(STUDENT_ID) +
               ",\"type\":\"active\",\"active\":" + (active ? "true" : "false") + "}";
  Serial.println("[TX] " + msg);
  wsClient.send(msg);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(PIN_HAPPY,    INPUT_PULLUP);
  pinMode(PIN_NEUTRAL,  INPUT_PULLUP);
  pinMode(PIN_SAD,      INPUT_PULLUP);
  pinMode(PIN_QUESTION, INPUT_PULLUP);
  pinMode(PIN_LIMIT,    INPUT_PULLUP);
  pinMode(PIN_LED,      OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.println("=== ESP32 Student Station ===");
  Serial.print("Student ID: "); Serial.println(STUDENT_ID);

  Serial.print("[WiFi] Connecting to "); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  Serial.print("[WiFi] Connected! IP: "); Serial.println(WiFi.localIP());

  wsClient.onEvent([](WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionClosed) {
      wsConnected = false;
      Serial.println("[WS] Disconnected.");
    }
  });

  connectWS();
}

void loop() {
  unsigned long now = millis();

  if (wsConnected) {
    wsClient.poll();
  } else if (now - lastReconnect > RECONNECT_MS) {
    lastReconnect = now;
    connectWS();
  }

  // ── Limit switch (hold to activate) ─────────────────────────
  bool limitReading = digitalRead(PIN_LIMIT);
  if (limitReading != lastLimit) {
    stationActive = (limitReading == LOW);
    digitalWrite(PIN_LED, stationActive ? HIGH : LOW);
    Serial.print("[Limit] Station "); Serial.println(stationActive ? "ACTIVE" : "INACTIVE");
    sendActive(stationActive);
  }
  lastLimit = limitReading;

  // ── Mood buttons (only when active) ──────────────────────────
  bool happyReading    = digitalRead(PIN_HAPPY);
  bool neutralReading  = digitalRead(PIN_NEUTRAL);
  bool sadReading      = digitalRead(PIN_SAD);
  bool questionReading = digitalRead(PIN_QUESTION);

  if (happyReading == LOW && lastHappy == HIGH) {
    Serial.println("[Btn] HAPPY");
    if (stationActive) sendVote("happy");
    else Serial.println("  (ignored - station inactive)");
  }
  if (neutralReading == LOW && lastNeutral == HIGH) {
    Serial.println("[Btn] NEUTRAL");
    if (stationActive) sendVote("neutral");
    else Serial.println("  (ignored - station inactive)");
  }
  if (sadReading == LOW && lastSad == HIGH) {
    Serial.println("[Btn] SAD");
    if (stationActive) sendVote("sad");
    else Serial.println("  (ignored - station inactive)");
  }
  if (questionReading == LOW && lastQuestion == HIGH) {
    Serial.println("[Btn] QUESTION");
    if (stationActive) sendQuestion();
    else Serial.println("  (ignored - station inactive)");
  }

  lastHappy    = happyReading;
  lastNeutral  = neutralReading;
  lastSad      = sadReading;
  lastQuestion = questionReading;

  delay(10);
}
