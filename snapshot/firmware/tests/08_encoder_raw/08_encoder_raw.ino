// Test 08: Raw Encoder Pin State Monitor
// Purpose: Show the actual digital state of each encoder's A and B pins
// to diagnose quadrature signal issues. Prints state changes as they happen.
//
// A healthy encoder should produce this quadrature sequence:
//   CW:  00 → 01 → 11 → 10 → 00  (Gray code)
//   CCW: 00 → 10 → 11 → 01 → 00  (reverse)
//
// If one channel is stuck or missing, you'll see only one bit changing.

// Test one encoder at a time — change this to select which encoder (1-8)
const int TEST_ENCODER = 1;

// Pin pairs for each encoder (original wiring — no A/B swap)
const int ENC_PINS[][2] = {
  {0, 1},   // Enc 1
  {2, 3},   // Enc 2
  {4, 5},   // Enc 3
  {6, 7},   // Enc 4
  {8, 9},   // Enc 5
  {10, 11}, // Enc 6
  {12, 13}, // Enc 7
  {14, 15}  // Enc 8
};

int pinA, pinB;
int lastA, lastB;
int stateCount = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinA = ENC_PINS[TEST_ENCODER - 1][0];
  pinB = ENC_PINS[TEST_ENCODER - 1][1];

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  lastA = digitalRead(pinA);
  lastB = digitalRead(pinB);

  Serial.print("=== Raw Encoder Pin Monitor — Encoder ");
  Serial.print(TEST_ENCODER);
  Serial.print(" (pins ");
  Serial.print(pinA);
  Serial.print("/");
  Serial.print(pinB);
  Serial.println(") ===");
  Serial.println();
  Serial.println("Turn the encoder slowly. Each state change is printed.");
  Serial.println("Healthy quadrature: both A and B columns should change.");
  Serial.println("If only one column changes, that channel is working; the other is stuck.");
  Serial.println();
  Serial.print("Initial state:  A=");
  Serial.print(lastA);
  Serial.print("  B=");
  Serial.println(lastB);
  Serial.println();
  Serial.println("  #   A  B  | Changed");
  Serial.println("  ──────────|────────");
}

void loop() {
  int a = digitalRead(pinA);
  int b = digitalRead(pinB);

  if (a != lastA || b != lastB) {
    stateCount++;

    Serial.print("  ");
    if (stateCount < 100) Serial.print(" ");
    if (stateCount < 10) Serial.print(" ");
    Serial.print(stateCount);
    Serial.print("  ");
    Serial.print(a);
    Serial.print("  ");
    Serial.print(b);
    Serial.print("  | ");

    if (a != lastA && b != lastB) {
      Serial.println("BOTH (missed a step — turning too fast)");
    } else if (a != lastA) {
      Serial.println("A changed");
    } else {
      Serial.println("B changed");
    }

    lastA = a;
    lastB = b;
  }
}
