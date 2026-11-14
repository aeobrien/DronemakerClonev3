// Test 10: Detent-Counting Encoder Test
// Purpose: Counts complete detent-to-detent movements using a state machine.
// Works with PEC16 encoders where the detent rest position (state 01) sits
// in the middle of one direction's quadrature cycle.
//
// From raw analysis with 100nF caps:
//   CW detent-to-detent: 01→00→01→11→01 (passes through rest state mid-cycle)
//   CCW detent-to-detent: 01→11→10→00→01 (clean cycle, doesn't revisit rest)
//
// Algorithm: track the last non-rest state visited. When we return to rest (01),
// determine direction from which states we passed through.
//
// Requires 100nF caps on A and B to ground for proper signal conditioning.

const int NUM_ENCODERS = 8;

const int PIN_A[] = {0, 2, 4, 6, 8, 10, 12, 14};
const int PIN_B[] = {1, 3, 5, 7, 9, 11, 13, 15};

volatile long encoderPos[NUM_ENCODERS];

// State encoding: (A << 1) | B
// State 0 = 00 (both low)
// State 1 = 01 (A low, B high) ← DETENT REST POSITION
// State 2 = 10 (A high, B low)
// State 3 = 11 (both high)

volatile uint8_t lastState[NUM_ENCODERS];
volatile bool seenState00[NUM_ENCODERS];  // visited state 00 since last detent
volatile bool seenState10[NUM_ENCODERS];  // visited state 10 since last detent
volatile bool seenState11[NUM_ENCODERS];  // visited state 11 since last detent

const uint8_t REST_STATE = 0b01;  // detent position: A=0, B=1

// ISR handlers — called on any transition on A or B
void handleEncoder(int idx) {
  uint8_t a = digitalReadFast(PIN_A[idx]);
  uint8_t b = digitalReadFast(PIN_B[idx]);
  uint8_t state = (a << 1) | b;

  if (state == lastState[idx]) return;  // no real change (noise)

  lastState[idx] = state;

  // Track which states we've visited
  if (state == 0b00) seenState00[idx] = true;
  if (state == 0b10) seenState10[idx] = true;
  if (state == 0b11) seenState11[idx] = true;

  // When we arrive at rest position, determine direction
  if (state == REST_STATE) {
    // CW path: 01→00→01→11→01 — visits 00 and 11
    // CCW path: 01→11→10→00→01 — visits 11, 10, and 00
    // Key differentiator: CCW visits state 10, CW does not

    if (seenState10[idx]) {
      // CCW — visited state 10
      encoderPos[idx]--;
    } else if (seenState00[idx] || seenState11[idx]) {
      // CW — visited 00 and/or 11 but NOT 10
      encoderPos[idx]++;
    }

    // Reset tracking for next detent
    seenState00[idx] = false;
    seenState10[idx] = false;
    seenState11[idx] = false;
  }
}

// Per-encoder ISR trampolines (both A and B trigger the same handler)
void isr0() { handleEncoder(0); }
void isr1() { handleEncoder(1); }
void isr2() { handleEncoder(2); }
void isr3() { handleEncoder(3); }
void isr4() { handleEncoder(4); }
void isr5() { handleEncoder(5); }
void isr6() { handleEncoder(6); }
void isr7() { handleEncoder(7); }

typedef void (*ISRFunc)();
ISRFunc isrFuncs[] = {isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7};

long displayPos[NUM_ENCODERS];

void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i = 0; i < NUM_ENCODERS; i++) {
    pinMode(PIN_A[i], INPUT_PULLUP);
    pinMode(PIN_B[i], INPUT_PULLUP);
    encoderPos[i] = 0;
    displayPos[i] = 0;

    uint8_t a = digitalRead(PIN_A[i]);
    uint8_t b = digitalRead(PIN_B[i]);
    lastState[i] = (a << 1) | b;
    seenState00[i] = false;
    seenState10[i] = false;
    seenState11[i] = false;

    attachInterrupt(digitalPinToInterrupt(PIN_A[i]), isrFuncs[i], CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_B[i]), isrFuncs[i], CHANGE);
  }

  Serial.println("=== Detent-Counting Encoder Test ===");
  Serial.println("One count per detent click.");
  Serial.println("CW = positive, CCW = negative (swap if backwards).");
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
