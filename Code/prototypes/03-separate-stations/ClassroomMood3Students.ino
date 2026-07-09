/*
  Classroom Mood Monitor — 3 Fixed Student Stations
  ==================================================
  Each student has their own 4 buttons on the breadboard.

  Student 1: D2=Happy  D3=Neutral  D4=Sad  D5=Question
  Student 2: D6=Happy  D7=Neutral  D8=Sad  D9=Question
  Student 3: D10=Happy D11=Neutral D12=Sad D13=Question

  Output:
    - LED matrix shows happy/neutral/sad face
    - Serial sends SCORE: and QUESTION: for the slideshow bridge
*/

#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

// ── Pin layout ───────────────────────────────────────────────────
const int NUM_STUDENTS = 3;

const int PIN_HAPPY[3]    = { 2,  6, 10 };
const int PIN_NEUTRAL[3]  = { 3,  7, 11 };
const int PIN_SAD[3]      = { 4,  8, 12 };
const int PIN_QUESTION[3] = { 5,  9, 13 };

const int NUM_PINS = 12;
const int ALL_PINS[12] = { 2,3,4,5, 6,7,8,9, 10,11,12,13 };

// ── Settings ─────────────────────────────────────────────────────
const int   QUESTION_THRESHOLD   = 2;        // 2 of 3 students must press
const unsigned long QUESTION_WINDOW_MS = 10000UL;
const float SMOOTHING            = 0.05f;
const unsigned long UPDATE_MS    = 100;

// ── State ────────────────────────────────────────────────────────
float         studentVote[3];
unsigned long lastVoteTime[3];
bool          questionActive  = false;
float         smoothedScore   = 0.0f;
float         targetScore     = 0.0f;

unsigned long questionPressLog[3];
int           questionPressCount = 0;

bool          lastPinState[12];
unsigned long lastUpdateTime = 0;

// ── LED Matrix faces ─────────────────────────────────────────────
byte happy[8][12] = {
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,0,0,0,0,0,0,0,1,0},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,1,0,0,0,0,1,0,0,1},
  {0,1,0,0,1,1,1,1,0,0,1,0},
  {0,0,1,1,0,0,0,0,1,1,0,0}
};

byte neutral[8][12] = {
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,0,0,0,0,0,0,0,1,0},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,1,1,1,1,1,0,0,1},
  {0,0,1,1,0,0,0,0,1,1,0,0}
};

byte sad[8][12] = {
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,0,0,0,0,0,0,0,1,0},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,0,0,0,1,0,1},
  {1,0,0,0,1,1,1,1,0,0,0,1},
  {0,1,0,1,0,0,0,0,1,0,1,0},
  {0,0,1,0,0,0,0,0,0,1,0,0}
};

byte questionFace[8][12] = {
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,0,0,1,1,0,0,0,1,0},
  {1,0,0,0,1,0,0,1,0,0,0,1},
  {1,0,0,0,0,0,0,1,0,0,0,1},
  {1,0,0,0,0,0,1,0,0,0,0,1},
  {1,0,0,0,0,1,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {0,0,1,1,0,0,1,0,0,1,1,0}
};

// ── Helpers ──────────────────────────────────────────────────────
void showFace(float score) {
  if (questionActive) {
    matrix.renderBitmap(questionFace, 8, 12);
  } else if (score > 3) {
    matrix.renderBitmap(happy, 8, 12);
  } else if (score < -3) {
    matrix.renderBitmap(sad, 8, 12);
  } else {
    matrix.renderBitmap(neutral, 8, 12);
  }
}

float recalcTargetScore() {
  float sum = 0.0f;
  int count = 0;
  for (int i = 0; i < NUM_STUDENTS; i++) {
    if (lastVoteTime[i] > 0) {
      sum += studentVote[i];
      count++;
    }
  }
  return count == 0 ? 0.0f : sum / count;
}

void recordVote(int student, float mood) {
  studentVote[student]  = mood;
  lastVoteTime[student] = millis();
  Serial.print("Student "); Serial.print(student + 1);
  Serial.print(" voted: ");
  if      (mood > 0)  Serial.println("HAPPY");
  else if (mood == 0) Serial.println("NEUTRAL");
  else                Serial.println("SAD");
}

void recordQuestion(int student) {
  unsigned long now = millis();

  // Clear expired presses
  int fresh = 0;
  for (int i = 0; i < questionPressCount; i++) {
    if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) {
      questionPressLog[fresh++] = questionPressLog[i];
    }
  }
  questionPressCount = fresh;

  if (questionPressCount < NUM_STUDENTS) {
    questionPressLog[questionPressCount++] = now;
  }

  int recent = questionPressCount;
  questionActive = (recent >= QUESTION_THRESHOLD);

  Serial.print("Student "); Serial.print(student + 1);
  Serial.print(" pressed question ("); Serial.print(recent);
  Serial.println(questionActive ? " total) - ACTIVE" : " total) - inactive");
}

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  delay(2000);

  matrix.begin();

  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(ALL_PINS[i], INPUT_PULLUP);
    lastPinState[i] = HIGH;
  }
  for (int i = 0; i < NUM_STUDENTS; i++) {
    studentVote[i]  = 0.0f;
    lastVoteTime[i] = 0;
  }

  showFace(0);

  Serial.println("=== 3 Student Mood Monitor Ready ===");
  Serial.println("S1: D2=H D3=N D4=S D5=?");
  Serial.println("S2: D6=H D7=N D8=S D9=?");
  Serial.println("S3: D10=H D11=N D12=S D13=?");
  Serial.println("=====================================");
}

// ── Loop ─────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Read all 12 pins
  for (int i = 0; i < NUM_PINS; i++) {
    bool reading = digitalRead(ALL_PINS[i]);
    if (reading != lastPinState[i]) {
      lastPinState[i] = reading;
      if (reading == LOW) {
        int pin = ALL_PINS[i];

        // Check each student's buttons
        for (int s = 0; s < NUM_STUDENTS; s++) {
          if      (pin == PIN_HAPPY[s])    recordVote(s,  10.0f);
          else if (pin == PIN_NEUTRAL[s])  recordVote(s,   0.0f);
          else if (pin == PIN_SAD[s])      recordVote(s, -10.0f);
          else if (pin == PIN_QUESTION[s]) recordQuestion(s);
        }
      }
    }
  }

  // Update score and display
  if (now - lastUpdateTime >= UPDATE_MS) {
    lastUpdateTime = now;

    targetScore    = recalcTargetScore();
    smoothedScore += (targetScore - smoothedScore) * SMOOTHING;

    // Check if question window expired
    if (questionActive) {
      int fresh = 0;
      for (int i = 0; i < questionPressCount; i++) {
        if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) fresh++;
      }
      if (fresh < QUESTION_THRESHOLD) questionActive = false;
    }

    showFace(smoothedScore);

    // Send to serial bridge
    static unsigned long lastPrint = 0;
    if (now - lastPrint >= 500) {
      lastPrint = now;
      Serial.print("SCORE:"); Serial.println(smoothedScore, 2);
      Serial.print("QUESTION:"); Serial.println(questionActive ? 1 : 0);
    }
  }

  delay(5);
}
