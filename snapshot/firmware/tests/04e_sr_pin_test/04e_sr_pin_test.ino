// Test 04e: Shift Register Per-Pin Test
// Purpose: Test every individual output (QA-QH) on every shift register (U1-U3)
// one at a time. For each pin, drives it LOW (LED on) while all others are HIGH (LED off).
//
// Use this with a multimeter on each output pin to confirm:
//   - Pin goes LOW (~0V) when activated
//   - Pin sits HIGH (~3.3V) when not activated
//
// The test runs automatically on a timer, or you can press any button to
// advance manually (hold a button at startup for manual mode).
//
// Shift register chain:
//   Teensy pin 32 (DATA) → U1 SER → U1 QH' → U2 SER → U2 QH' → U3 SER
//   Byte shifted FIRST ends up in U3 (farthest), LAST ends up in U1 (nearest)
//
// 74HC595 pin reference:
//   Pin 15 = QA (output 0)
//   Pin  1 = QB (output 1)
//   Pin  2 = QC (output 2)
//   Pin  3 = QD (output 3)
//   Pin  4 = QE (output 4)
//   Pin  5 = QF (output 5)
//   Pin  6 = QG (output 6)
//   Pin  7 = QH (output 7)

const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int NUM_BUTTONS = 8;

// Which IC pin corresponds to each output bit
const int OUTPUT_TO_PIN[] = {15, 1, 2, 3, 4, 5, 6, 7}; // QA=pin15, QB=pin1, etc.
const char* OUTPUT_NAMES[] = {"QA", "QB", "QC", "QD", "QE", "QF", "QG", "QH"};

// Board roles (from build checklist)
const char* CHIP_ROLES[] = {"Green cathodes", "Red cathodes", "Blue cathodes"};
// U1 = nearest (byte 3, shifted last)  = Green
// U2 = middle  (byte 2, shifted second) = Red
// U3 = farthest (byte 1, shifted first) = Blue

int currentStep = -1;
const int TOTAL_STEPS = 24; // 3 chips × 8 outputs

void sendRaw(byte b1, byte b2, byte b3) {
  // b1 shifted first → ends in U3 (farthest, blue)
  // b2 shifted second → ends in U2 (middle, red)
  // b3 shifted last → ends in U1 (nearest, green)
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

void allOff() {
  sendRaw(0xFF, 0xFF, 0xFF);
}

bool anyButtonPressed() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(BUTTON_PINS[i]) == LOW) return true;
  }
  return false;
}

void waitForRelease() {
  while (anyButtonPressed()) delay(10);
  delay(50);
}

void waitForPress() {
  while (!anyButtonPressed()) delay(10);
  delay(50);
  waitForRelease();
}

void activateStep(int step) {
  // step 0-7:   U1 (nearest,  green) outputs QA-QH → byte 3
  // step 8-15:  U2 (middle,   red)   outputs QA-QH → byte 2
  // step 16-23: U3 (farthest, blue)  outputs QA-QH → byte 1

  int chip = step / 8;      // 0=U1, 1=U2, 2=U3
  int output = step % 8;    // 0=QA, 1=QB, ... 7=QH

  byte b1 = 0xFF, b2 = 0xFF, b3 = 0xFF; // all off

  byte activeByte = ~(1 << output); // pull one bit LOW

  switch (chip) {
    case 0: b3 = activeByte; break; // U1 = last shifted = byte 3
    case 1: b2 = activeByte; break; // U2 = middle = byte 2
    case 2: b1 = activeByte; break; // U3 = first shifted = byte 1
  }

  sendRaw(b1, b2, b3);

  int icPin = OUTPUT_TO_PIN[output];

  Serial.println("────────────────────────────────────");
  Serial.print("Step ");
  Serial.print(step + 1);
  Serial.print("/");
  Serial.print(TOTAL_STEPS);
  Serial.print(" │ U");
  Serial.print(chip + 1);
  Serial.print(" (");
  Serial.print(CHIP_ROLES[chip]);
  Serial.print(") │ ");
  Serial.print(OUTPUT_NAMES[output]);
  Serial.print(" │ IC pin ");
  Serial.println(icPin);
  Serial.println();
  Serial.print("  MEASURE: U");
  Serial.print(chip + 1);
  Serial.print(" pin ");
  Serial.print(icPin);
  Serial.println(" should read ~0V (LOW)");
  Serial.println("  All other outputs should read ~3.3V (HIGH)");
  Serial.println();

  // Also tell them which button LED should respond
  if (output >= 1 && output <= 7) {
    // Outputs 1-7 typically map to buttons (bit 0 may be unused)
    // But we don't know the exact mapping — just tell them what to look for
  }

  Serial.println("  Press any button to advance...");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  allOff();

  Serial.println("╔══════════════════════════════════════════════════╗");
  Serial.println("║     SHIFT REGISTER PER-PIN DIAGNOSTIC TEST      ║");
  Serial.println("╠══════════════════════════════════════════════════╣");
  Serial.println("║ Tests each of the 24 shift register outputs     ║");
  Serial.println("║ individually. For each step, exactly ONE output ║");
  Serial.println("║ is driven LOW — all others remain HIGH.         ║");
  Serial.println("║                                                 ║");
  Serial.println("║ Use multimeter (DC volts, black on GND):        ║");
  Serial.println("║   Active pin  → should read ~0V                 ║");
  Serial.println("║   Other pins  → should read ~3.3V               ║");
  Serial.println("║                                                 ║");
  Serial.println("║ If an LED lights up, note which button + colour ║");
  Serial.println("║                                                 ║");
  Serial.println("║ Press any button to advance each step.          ║");
  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();

  // Phase 0: verify all off
  Serial.println("── PHASE 0: ALL OUTPUTS HIGH (ALL OFF) ──");
  Serial.println("Verify: no LEDs are lit, all outputs read ~3.3V");
  Serial.println();
  Serial.println("Press any button to begin...");
  waitForPress();
}

void loop() {
  currentStep++;

  if (currentStep >= TOTAL_STEPS) {
    allOff();
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║              ALL 24 PINS TESTED                 ║");
    Serial.println("╠══════════════════════════════════════════════════╣");
    Serial.println("║ If any pin did not go LOW when activated:       ║");
    Serial.println("║   → Cold solder joint on that IC socket pin     ║");
    Serial.println("║   → Or damaged IC (try swapping to spare)       ║");
    Serial.println("║                                                 ║");
    Serial.println("║ If ALL pins on one chip failed:                 ║");
    Serial.println("║   → Check VCC/GND/OE/SRCLR on that chip        ║");
    Serial.println("║   → Check daisy chain to that chip (SER pin 14) ║");
    Serial.println("║                                                 ║");
    Serial.println("║ If LEDs lit up, note the colour-to-chip mapping ║");
    Serial.println("║ and compare with the build checklist.           ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Resetting in 10 seconds...");
    delay(10000);
    currentStep = -1;
    return;
  }

  // Print chip header at start of each chip
  if (currentStep % 8 == 0) {
    int chip = currentStep / 8;
    Serial.println();
    Serial.print("══ TESTING U");
    Serial.print(chip + 1);
    Serial.print(" — ");
    Serial.print(CHIP_ROLES[chip]);
    Serial.println(" ══");
    Serial.println();
  }

  activateStep(currentStep);

  waitForPress();
}
