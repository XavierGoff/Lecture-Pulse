const int buttonPins[6] = {2, 3, 4, 5, 6, 7};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  Serial.print("Buttons pressed: ");

  bool anyPressed = false;

  for (int i = 0; i < 6; i++) {
    int buttonState = digitalRead(buttonPins[i]);

    // With INPUT_PULLUP:
    // HIGH = not pressed
    // LOW = pressed
    if (buttonState == LOW) {
      Serial.print(i + 1);
      Serial.print(" ");
      anyPressed = true;
    }
  }

  if (!anyPressed) {
    Serial.print("none");
  }

  Serial.println();

  delay(100);
}
