/*
  Judging System — Master Relay ESP32
  =====================================
  No buttons. Receives ESP-NOW from judge pads, sends SCORE/QUESTION to laptop via USB serial.
*/

#include <WiFi.h>
#include <esp_now.h>

const int TOTAL_STUDENTS     = 2;
const int QUESTION_THRESHOLD = 2;
const unsigned long QUESTION_WINDOW_MS = 10000UL;
const float SMOOTHING        = 0.05f;

float         studentVote[3];
bool          studentVoted[3];
bool          studentActive[3];
unsigned long questionPressLog[2];
int           questionPressCount = 0;
float         smoothedScore      = 0.0f;
unsigned long lastUpdateTime     = 0;

typedef struct {
  int  studentId;
  char type[16];
  char mood[16];
  bool active;
} StudentMessage;

float calcTargetScore() {
  float sum = 0.0f;
  int count = 0;
  for (int i = 1; i <= TOTAL_STUDENTS; i++) {
    if (studentVoted[i]) {
      sum += studentVote[i];
      count++;
    }
  }
  return count == 0 ? 0.0f : sum / count;
}

bool isQuestionActive() {
  unsigned long now = millis();
  int recent = 0;
  for (int i = 0; i < questionPressCount; i++) {
    if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) recent++;
  }
  return recent >= QUESTION_THRESHOLD;
}

void recordQuestion() {
  unsigned long now = millis();
  int fresh = 0;
  for (int i = 0; i < questionPressCount; i++) {
    if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) {
      questionPressLog[fresh++] = questionPressLog[i];
    }
  }
  questionPressCount = fresh;
  if (questionPressCount < TOTAL_STUDENTS) {
    questionPressLog[questionPressCount++] = now;
  }
}

void onDataReceived(const esp_now_recv_info* info, const uint8_t* data, int len) {
  if (len != sizeof(StudentMessage)) return;
  StudentMessage msg;
  memcpy(&msg, data, sizeof(msg));

  int s = msg.studentId;
  if (s < 1 || s > TOTAL_STUDENTS) return;

  if (strcmp(msg.type, "vote") == 0) {
    if      (strcmp(msg.mood, "happy")   == 0) studentVote[s] =  10.0f;
    else if (strcmp(msg.mood, "neutral") == 0) studentVote[s] =   0.0f;
    else if (strcmp(msg.mood, "sad")     == 0) studentVote[s] = -10.0f;
    studentVoted[s] = true;
    Serial.printf("[ESP-NOW] Judge %d voted %s\n", s, msg.mood);
  } else if (strcmp(msg.type, "question") == 0) {
    recordQuestion();
    Serial.printf("[ESP-NOW] Judge %d pressed ?\n", s);
  } else if (strcmp(msg.type, "active") == 0) {
    studentActive[s] = msg.active;
    Serial.printf("[ESP-NOW] Judge %d %s\n", s, msg.active ? "ACTIVE" : "INACTIVE");
  }
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  for (int i = 0; i <= TOTAL_STUDENTS; i++) {
    studentVote[i]   = 0.0f;
    studentVoted[i]  = false;
    studentActive[i] = false;
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init failed!");
  } else {
    Serial.println("[ESP-NOW] Ready");
    esp_now_register_recv_cb(onDataReceived);
  }

  Serial.println("=== Judge Relay Ready ===");
  Serial.print("MAC: "); Serial.println(WiFi.macAddress());
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdateTime >= 500) {
    lastUpdateTime = now;
    float target = calcTargetScore();
    smoothedScore += (target - smoothedScore) * SMOOTHING;
    Serial.print("SCORE:"); Serial.println(smoothedScore, 2);
    Serial.print("QUESTION:"); Serial.println(isQuestionActive() ? 1 : 0);
  }

  delay(5);
}
