// Test 3: Encoder Test
// Purpose: Verify encoder rotation (A/B channels) and push switches (pins 16-23).
// Connect one or more encoders via JST cables to J1-J8.
// Open serial monitor — turning should show position changes, pushing should show PRESSED.
//
// Requires: Encoder library (Paul Stoffregen) — install via Arduino Library Manager.

#include <Encoder.h>

// Encoder A/B pairs on pins 0-15
Encoder enc1(1, 0);   // A/B swapped — loom wiring has channels reversed
Encoder enc2(3, 2);
Encoder enc3(5, 4);
Encoder enc4(7, 6);
Encoder enc5(9, 8);
Encoder enc6(11, 10);
Encoder enc7(13, 12);
Encoder enc8(15, 14);

Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7, &enc8};

// Push switch pins
const int PUSH_PINS[] = {16, 17, 18, 19, 20, 21, 22, 23};
const int NUM_ENCODERS = 8;

long lastPosition[NUM_ENCODERS];
bool lastPush[NUM_ENCODERS];

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_ENCODERS; i++) {
    pinMode(PUSH_PINS[i], INPUT_PULLUP);
    lastPosition[i] = 0;
    lastPush[i] = HIGH;
  }

  Serial.println("=== Encoder Test ===");
  Serial.println("Turn any encoder (J1-J8) to see position changes.");
  Serial.println("Push any encoder shaft to see PRESSED/RELEASED.");
  Serial.println();
}

void loop() {
  for (int i = 0; i < NUM_ENCODERS; i++) {
    // Check rotation
    long pos = encoders[i]->read();
    if (pos != lastPosition[i]) {
      int delta = pos - lastPosition[i];
      Serial.print("Encoder ");
      Serial.print(i + 1);
      Serial.print(": position=");
      Serial.print(pos);
      Serial.print(" (delta=");
      Serial.print(delta > 0 ? "+" : "");
      Serial.print(delta);
      Serial.println(")");
      lastPosition[i] = pos;
    }

    // Check push switch
    bool push = digitalRead(PUSH_PINS[i]);
    if (push != lastPush[i]) {
      delay(5); // debounce
      push = digitalRead(PUSH_PINS[i]);
      if (push != lastPush[i]) {
        Serial.print("Encoder ");
        Serial.print(i + 1);
        Serial.print(" push (pin ");
        Serial.print(PUSH_PINS[i]);
        Serial.print("): ");
        Serial.println(push == LOW ? "PRESSED" : "RELEASED");
        lastPush[i] = push;
      }
    }
  }
}
