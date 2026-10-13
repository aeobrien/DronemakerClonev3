// Test 09: State-Aware Encoder Test (v3)
// Purpose: Encoder reading for PEC16 encoders that produce one-channel-per-direction
// signals instead of standard interleaved quadrature.
//
// From raw pin analysis:
//   CW:  B toggles while A=0  (states 00↔01)
//   CCW: A toggles while B=1  (states 01↔11)
//
// On each falling edge, check the other channel and debounce:
//   B falls AND A==0 → CW step  (+1)
//   A falls AND B==1 → CCW step (-1)
//   Anything else    → ignore (transition noise)
//   Any edge within 3ms of last counted edge → ignore (contact bounce)

const int NUM_ENCODERS = 8;

const int PIN_A[] = {0, 2, 4, 6, 8, 10, 12, 14};
const int PIN_B[] = {1, 3, 5, 7, 9, 11, 13, 15};

volatile long encoderPos[NUM_ENCODERS];
volatile unsigned long lastEdgeTime[NUM_ENCODERS];  // single debounce timer per encoder

const unsigned long DEBOUNCE_US = 3000;  // 3ms — longer than contact bounce, shorter than fastest turn

// ISR for channel A — only counts on falling edge with B==1
void handleA(int idx) {
  unsigned long now = micros();
  if (now - lastEdgeTime[idx] < DEBOUNCE_US) return;
  if (digitalReadFast(PIN_A[idx]) == LOW && digitalReadFast(PIN_B[idx]) == HIGH) {
    encoderPos[idx]--;
    lastEdgeTime[idx] = now;
  }
}

// ISR for channel B — only counts on falling edge with A==0
void handleB(int idx) {
  unsigned long now = micros();
  if (now - lastEdgeTime[idx] < DEBOUNCE_US) return;
  if (digitalReadFast(PIN_B[idx]) == LOW && digitalReadFast(PIN_A[idx]) == LOW) {
    encoderPos[idx]++;
    lastEdgeTime[idx] = now;
  }
}

// Per-encoder ISR trampolines
void isrA0() { handleA(0); }  void isrB0() { handleB(0); }
void isrA1() { handleA(1); }  void isrB1() { handleB(1); }
void isrA2() { handleA(2); }  void isrB2() { handleB(2); }
void isrA3() { handleA(3); }  void isrB3() { handleB(3); }
void isrA4() { handleA(4); }  void isrB4() { handleB(4); }
void isrA5() { handleA(5); }  void isrB5() { handleB(5); }
void isrA6() { handleA(6); }  void isrB6() { handleB(6); }
void isrA7() { handleA(7); }  void isrB7() { handleB(7); }

typedef void (*ISRFunc)();
ISRFunc isrAFuncs[] = {isrA0, isrA1, isrA2, isrA3, isrA4, isrA5, isrA6, isrA7};
ISRFunc isrBFuncs[] = {isrB0, isrB1, isrB2, isrB3, isrB4, isrB5, isrB6, isrB7};

long displayPos[NUM_ENCODERS];

void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i = 0; i < NUM_ENCODERS; i++) {
    pinMode(PIN_A[i], INPUT_PULLUP);
    pinMode(PIN_B[i], INPUT_PULLUP);
    encoderPos[i] = 0;
    displayPos[i] = 0;
    lastEdgeTime[i] = 0;

    attachInterrupt(digitalPinToInterrupt(PIN_A[i]), isrAFuncs[i], FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_B[i]), isrBFuncs[i], FALLING);
  }

  Serial.println("=== State-Aware Encoder Test v3 (debounced) ===");
  Serial.println("CW should increase, CCW should decrease.");
  Serial.println("One count per detent click.");
  Serial.println();
}

void loop() {
  for (int i = 0; i < NUM_ENCODERS; i++) {
    noInterrupts();
    long pos = encoderPos[i];
    interrupts();

    if (pos != displayPos[i]) {
      long delta = pos - displayPos[i];
      Serial.print("Enc ");
      Serial.print(i + 1);
      Serial.print(" (J");
      Serial.print(i + 1);
      Serial.print("): pos=");
      Serial.print(pos);
      Serial.print(" delta=");
      Serial.print(delta > 0 ? "+" : "");
      Serial.println(delta);
      displayPos[i] = pos;
    }
  }
  delay(10);
}
