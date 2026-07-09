/*
  Judging System — Judge Pad (Slave ESP32)
  =========================================
  Change STUDENT_ID to 1 or 2 for each judge pad.
  Update masterMAC with the relay ESP32's MAC address.

  Wiring:
    GPIO 13 — Happy button
    GPIO 12 — Neutral button
    GPIO 14 — Sad button
    GPIO 27 — Question button
    GPIO 26 — Limit switch
    GPIO 25 — Green LED
*/

#include <WiFi.h>
#include <esp_now.h>

// ── Change for each judge pad ───────────────────────────────────
const int STUDENT_ID = 2;  // 1 or 2

// ── Update with relay ESP32's MAC address ───────────────────────
uint8_t masterMAC[] = {0xEC, 0x64, 0xC9, 0x86, 0xB2, 0x1C};// ───────────────────────────────────────────────────────────────

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

typedef struct {
  int  studentId;
  char type[16];
  char mood[16];
  bool active;
} StudentMessage;

void sendMessage(const char* type, const char* mood = "", bool active = false) {
  StudentMessage msg;
  msg.studentId = STUDENT_ID;
  strncpy(msg.type, type, sizeof(msg.type));
  strncpy(msg.mood, mood, sizeof(msg.mood));
  msg.active = active;
  esp_now_send(masterMAC, (uint8_t*)&msg, sizeof(msg));
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  pinMode(PIN_HAPPY,    INPUT_PULLUP);
  pinMode(PIN_NEUTRAL,  INPUT_PULLUP);
  pinMode(PIN_SAD,      INPUT_PULLUP);
  pinMode(PIN_QUESTION, INPUT_PULLUP);
  pinMode(PIN_LIMIT,    INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init failed!");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW); // off by default (active LOW)

  Serial.printf("=== Judge Pad — ID %d ===\n", STUDENT_ID);
  Serial.println("[ESP-NOW] Ready");
}

void loop() {
  bool limitReading = digitalRead(PIN_LIMIT);
  if (limitReading != lastLimit) {
    delay(50);
    limitReading = digitalRead(PIN_LIMIT);
    if (limitReading != lastLimit) {
      stationActive = (limitReading == LOW);
      digitalWrite(PIN_LED, stationActive ? HIGH : LOW);
      Serial.println(stationActive ? "[Limit] ACTIVE" : "[Limit] INACTIVE");
      sendMessage("active", "", stationActive);
      lastLimit = limitReading;
    }
  }

  bool happyReading    = digitalRead(PIN_HAPPY);
  bool neutralReading  = digitalRead(PIN_NEUTRAL);
  bool sadReading      = digitalRead(PIN_SAD);
  bool questionReading = digitalRead(PIN_QUESTION);

  if (happyReading == LOW && lastHappy == HIGH) {
    Serial.println("[Btn] HAPPY");
    if (stationActive) sendMessage("vote", "happy");
    else Serial.println("  (ignored - inactive)");
  }
  if (neutralReading == LOW && lastNeutral == HIGH) {
    Serial.println("[Btn] NEUTRAL");
    if (stationActive) sendMessage("vote", "neutral");
    else Serial.println("  (ignored - inactive)");
  }
  if (sadReading == LOW && lastSad == HIGH) {
    Serial.println("[Btn] SAD");
    if (stationActive) sendMessage("vote", "sad");
    else Serial.println("  (ignored - inactive)");
  }
  if (questionReading == LOW && lastQuestion == HIGH) {
    Serial.println("[Btn] QUESTION");
    if (stationActive) sendMessage("question");
    else Serial.println("  (ignored - inactive)");
  }

  lastHappy    = happyReading;
  lastNeutral  = neutralReading;
  lastSad      = sadReading;
  lastQuestion = questionReading;

  delay(10);
}
