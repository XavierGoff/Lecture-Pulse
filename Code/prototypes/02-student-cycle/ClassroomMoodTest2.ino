/*
  Classroom Mood Monitor — 4 Student Test
  ========================================
  D2 = Happy (+10)
  D3 = Neutral (0)
  D4 = Sad (-10)
  D5 = Question mark
  D6 = Prev student
  D7 = Next student

  Output: LED matrix shows smiley/neutral/sad face based on class mood.
  Serial Monitor shows full status every second.
*/

#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

const int PIN_HAPPY    = 2;
const int PIN_NEUTRAL  = 3;
const int PIN_SAD      = 4;
const int PIN_QUESTION = 5;
const int PIN_PREV     = 6;
const int PIN_NEXT     = 7;
const int NUM_PINS     = 6;
const int ALL_PINS[]   = { PIN_HAPPY, PIN_NEUTRAL, PIN_SAD, PIN_QUESTION, PIN_PREV, PIN_NEXT };

const int   TOTAL_STUDENTS        = 4;
const int   QUESTION_THRESHOLD    = 2;
const unsigned long QUESTION_WINDOW_MS = 10000UL;
const float SMOOTHING             = 0.05f;
const unsigned long UPDATE_MS     = 100;

int   currentStudent   = 1;
float smoothedScore    = 0.0f;
float targetScore      = 0.0f;
bool  questionActive   = false;

float         studentVote[5];
unsigned long lastVoteTime[5];
unsigned long questionPressLog[4];
int           questionPressCount = 0;

bool          lastPinState[NUM_PINS];
unsigned long lastUpdateTime = 0;

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

byte question[8][12] = {
  {0,0,1,1,1,1,1,1,1,1,0,0},
  {0,1,0,0,0,1,1,0,0,0,1,0},
  {1,0,0,0,1,0,0,1,0,0,0,1},
  {1,0,0,0,0,0,0,1,0,0,0,1},
  {1,0,0,0,0,0,1,0,0,0,0,1},
  {1,0,0,0,0,1,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1},
  {0,0,1,1,0,0,1,0,0,1,1,0}
};

void showFace(float score) {
  if (questionActive) {
    matrix.renderBitmap(question, 8, 12);
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
  for (int i = 1; i <= TOTAL_STUDENTS; i++) {
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
  Serial.print("Student "); Serial.print(student);
  Serial.print(" voted: "); Serial.println(mood);
}

void recordQuestion() {
  unsigned long now = millis();

  // Clear out expired presses first
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
  int recent = 0;
  for (int i = 0; i < questionPressCount; i++) {
    if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) recent++;
  }
  questionActive = (recent >= QUESTION_THRESHOLD);
  Serial.print("Question presses in window: "); Serial.print(recent);
  Serial.println(questionActive ? " - ACTIVE" : " - inactive");
}

void handleButtonPress(int pin) {
  if      (pin == PIN_HAPPY)    { Serial.println("Button: HAPPY");    recordVote(currentStudent,  10.0f); }
  else if (pin == PIN_NEUTRAL)  { Serial.println("Button: NEUTRAL");  recordVote(currentStudent,   0.0f); }
  else if (pin == PIN_SAD)      { Serial.println("Button: SAD");      recordVote(currentStudent, -10.0f); }
  else if (pin == PIN_QUESTION) { Serial.println("Button: QUESTION"); recordQuestion(); }
  else if (pin == PIN_PREV) {
    currentStudent--;
    if (currentStudent < 1) currentStudent = TOTAL_STUDENTS;
    Serial.print("Student -> "); Serial.println(currentStudent);
  }
  else if (pin == PIN_NEXT) {
    currentStudent++;
    if (currentStudent > TOTAL_STUDENTS) currentStudent = 1;
    Serial.print("Student -> "); Serial.println(currentStudent);
  }
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  matrix.begin();

  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(ALL_PINS[i], INPUT_PULLUP);
    lastPinState[i] = HIGH;
  }
  for (int i = 0; i <= TOTAL_STUDENTS; i++) {
    studentVote[i]  = 0.0f;
    lastVoteTime[i] = 0;
  }

  showFace(0);
  Serial.println("=== Classroom Mood Monitor Ready ===");
  Serial.println("D2=Happy D3=Neutral D4=Sad D5=? D6=Prev D7=Next");
  Serial.println("====================================");
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_PINS; i++) {
    bool reading = digitalRead(ALL_PINS[i]);
    if (reading != lastPinState[i]) {
      lastPinState[i] = reading;
      if (reading == LOW) {
        handleButtonPress(ALL_PINS[i]);
      }
    }
  }

  if (now - lastUpdateTime >= UPDATE_MS) {
    lastUpdateTime = now;

    targetScore   = recalcTargetScore();
    smoothedScore += (targetScore - smoothedScore) * SMOOTHING;

    if (questionActive) {
      int recent = 0;
      for (int i = 0; i < questionPressCount; i++) {
        if (now - questionPressLog[i] <= QUESTION_WINDOW_MS) recent++;
      }
      if (recent < QUESTION_THRESHOLD) questionActive = false;
    }

    showFace(smoothedScore);

    static unsigned long lastPrint = 0;
    if (now - lastPrint >= 500) {
      lastPrint = now;
      Serial.print("SCORE:"); Serial.println(smoothedScore, 2);
      Serial.print("QUESTION:"); Serial.println(questionActive ? 1 : 0);
    }
  }

  delay(5);
}
