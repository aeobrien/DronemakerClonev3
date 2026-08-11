// Test 07: Live Input Monitor
// Purpose: Freeform diagnostic — shows all encoder movement, encoder pushes,
// and button presses in real time. No sequence, no timeouts. Just plug things
// in and watch the serial monitor.
//
// Requires: Encoder library (Paul Stoffregen)

#include <Encoder.h>

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int PUSH_PINS[]   = {16, 17, 18, 19, 20, 21, 22, 23};

Encoder enc1(0, 1);
Encoder enc2(2, 3);
Encoder enc3(4, 5);
Encoder enc4(6, 7);
Encoder enc5(8, 9);
Encoder enc6(10, 11);
Encoder enc7(12, 13);
Encoder enc8(14, 15);
Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7, &enc8};

long lastPosition[8];
bool lastButton[8];
bool lastPush[8];

void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(PUSH_PINS[i], INPUT_PULLUP);
    lastButton[i] = HIGH;
    lastPush[i] = HIGH;
    lastPosition[i] = 0;
  }

  Serial.println("=== Live Input Monitor ===");
  Serial.println("All inputs shown in real time. Do whatever you like.");
  Serial.println();
}

void loop() {
  // Buttons
  for (int i = 0; i < 8; i++) {
    bool state = digitalRead(BUTTON_PINS[i]);
    if (state != lastButton[i]) {
      delay(5);
      state = digitalRead(BUTTON_PINS[i]);
      if (state != lastButton[i]) {
        Serial.print("Button ");
        Serial.print(i + 1);
        Serial.print(" (J");
        Serial.print(i + 9);
        Serial.print("): ");
        Serial.println(state == LOW ? "PRESSED" : "RELEASED");
        lastButton[i] = state;
      }
    }
  }

  // Encoder pushes
  for (int i = 0; i < 8; i++) {
    bool state = digitalRead(PUSH_PINS[i]);
    if (state != lastPush[i]) {
      delay(5);
      state = digitalRead(PUSH_PINS[i]);
      if (state != lastPush[i]) {
        Serial.print("Enc ");
        Serial.print(i + 1);
        Serial.print(" push (J");
        Serial.print(i + 1);
        Serial.print("): ");
        Serial.println(state == LOW ? "PRESSED" : "RELEASED");
        lastPush[i] = state;
      }
    }
  }

  // Encoder rotation
  for (int i = 0; i < 8; i++) {
    long pos = encoders[i]->read();
    if (pos != lastPosition[i]) {
      long delta = pos - lastPosition[i];
      Serial.print("Enc ");
      Serial.print(i + 1);
      Serial.print(" (J");
      Serial.print(i + 1);
      Serial.print("): pos=");
      Serial.print(pos);
      Serial.print(" delta=");
      Serial.print(delta > 0 ? "+" : "");
      Serial.println(delta);
      lastPosition[i] = pos;
    }
  }
}
