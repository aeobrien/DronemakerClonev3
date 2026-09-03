// Test 06: Systematic Input Diagnostic
// Purpose: Test every encoder, encoder push, and button one at a time
// in a guided sequence. Designed to identify exactly which inputs work
// and which don't, so problems can be traced to looms, sockets, or PCB.
//
// Three phases:
//   Phase 1: Buttons (J9-J16, pins 24-31) — press and release each
//   Phase 2: Encoder push switches (J1-J8, pins 16-23) — press and release each
//   Phase 3: Encoders (J1-J8, pins 0-15) — turn each CW then CCW
//
// Serial monitor shows pass/fail for each input. At the end, a summary
// lists all failures for systematic debugging.
//
// Requires: Encoder library (Paul Stoffregen)

#include <Encoder.h>

// --- Pin assignments ---
const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int PUSH_PINS[]   = {16, 17, 18, 19, 20, 21, 22, 23};

Encoder enc1(1, 0);   // A/B swapped — loom wiring has channels reversed
Encoder enc2(3, 2);
Encoder enc3(5, 4);
Encoder enc4(7, 6);
Encoder enc5(9, 8);
Encoder enc6(11, 10);
Encoder enc7(13, 12);
Encoder enc8(15, 14);
Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7, &enc8};

// --- Results tracking ---
enum Result { UNTESTED, PASS, FAIL, PARTIAL };

Result buttonResults[8];
Result pushResults[8];
Result encoderResults[8];
// Extra detail for encoders
bool encoderCW[8];
bool encoderCCW[8];

// For detecting spurious triggers on other inputs
int spuriousButtonCount = 0;
int spuriousPushCount = 0;
int spuriousEncoderCount = 0;

const unsigned long TEST_TIMEOUT = 10000; // 10 seconds per input

// --- Helpers ---

void printDivider() {
  Serial.println("────────────────────────────────────────────────");
}

void printHeader(const char* text) {
  Serial.println();
  Serial.println("════════════════════════════════════════════════");
  Serial.print("  ");
  Serial.println(text);
  Serial.println("════════════════════════════════════════════════");
  Serial.println();
}

// Check if any button OTHER than the expected one is triggering
void checkSpuriousButtons(int expectedIndex) {
  for (int i = 0; i < 8; i++) {
    if (i == expectedIndex) continue;
    if (digitalRead(BUTTON_PINS[i]) == LOW) {
      Serial.print("  ⚠ SPURIOUS: Button ");
      Serial.print(i + 1);
      Serial.print(" (J");
      Serial.print(i + 9);
      Serial.println(") also triggered!");
      spuriousButtonCount++;
    }
  }
}

void checkSpuriousPushes(int expectedIndex) {
  for (int i = 0; i < 8; i++) {
    if (i == expectedIndex) continue;
    if (digitalRead(PUSH_PINS[i]) == LOW) {
      Serial.print("  ⚠ SPURIOUS: Encoder ");
      Serial.print(i + 1);
      Serial.print(" push (J");
      Serial.print(i + 1);
      Serial.println(") also triggered!");
      spuriousPushCount++;
    }
  }
}

void checkSpuriousEncoders(int expectedIndex) {
  for (int i = 0; i < 8; i++) {
    if (i == expectedIndex) continue;
    long pos = encoders[i]->read();
    if (pos != 0) {
      Serial.print("  ⚠ SPURIOUS: Encoder ");
      Serial.print(i + 1);
      Serial.print(" (J");
      Serial.print(i + 1);
      Serial.print(") moved to ");
      Serial.println(pos);
      encoders[i]->write(0);
      spuriousEncoderCount++;
    }
  }
}

// Wait for a pin to go LOW (pressed), with timeout.
// Returns true if pressed within timeout.
bool waitForPress(int pin, unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (digitalRead(pin) == LOW) {
      delay(10); // debounce
      if (digitalRead(pin) == LOW) return true;
    }
  }
  return false;
}

// Wait for a pin to go HIGH (released), with timeout.
bool waitForRelease(int pin, unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (digitalRead(pin) == HIGH) {
      delay(10);
      if (digitalRead(pin) == HIGH) return true;
    }
  }
  return false;
}

// --- Phase 1: Buttons ---

void testButton(int index) {
  int pin = BUTTON_PINS[index];
  int jHeader = index + 9;

  printDivider();
  Serial.print("Button ");
  Serial.print(index + 1);
  Serial.print(" — J");
  Serial.print(jHeader);
  Serial.print(" (pin ");
  Serial.print(pin);
  Serial.println(")");
  Serial.println("  Press and release the button. (10s timeout)");

  // Check initial state
  if (digitalRead(pin) == LOW) {
    Serial.println("  ⚠ Pin already LOW before pressing — stuck or shorted?");
  }

  // Wait for press
  if (!waitForPress(pin, TEST_TIMEOUT)) {
    Serial.println("  ✗ TIMEOUT — no press detected");
    buttonResults[index] = FAIL;
    return;
  }

  Serial.println("  ↓ PRESSED");
  checkSpuriousButtons(index);
  checkSpuriousPushes(-1); // none expected
  checkSpuriousEncoders(-1);

  // Wait for release
  if (!waitForRelease(pin, TEST_TIMEOUT)) {
    Serial.println("  ✗ TIMEOUT — button stuck LOW (not releasing)");
    buttonResults[index] = FAIL;
    return;
  }

  Serial.println("  ↑ RELEASED");
  Serial.println("  ✓ PASS");
  buttonResults[index] = PASS;
}

// --- Phase 2: Encoder pushes ---

void testPush(int index) {
  int pin = PUSH_PINS[index];
  int jHeader = index + 1;

  printDivider();
  Serial.print("Encoder ");
  Serial.print(index + 1);
  Serial.print(" push — J");
  Serial.print(jHeader);
  Serial.print(" (pin ");
  Serial.print(pin);
  Serial.println(")");
  Serial.println("  Push and release the encoder shaft. (10s timeout)");

  if (digitalRead(pin) == LOW) {
    Serial.println("  ⚠ Pin already LOW before pressing — stuck or shorted?");
  }

  if (!waitForPress(pin, TEST_TIMEOUT)) {
    Serial.println("  ✗ TIMEOUT — no press detected");
    pushResults[index] = FAIL;
    return;
  }

  Serial.println("  ↓ PRESSED");
  checkSpuriousPushes(index);
  checkSpuriousButtons(-1);
  checkSpuriousEncoders(-1);

  if (!waitForRelease(pin, TEST_TIMEOUT)) {
    Serial.println("  ✗ TIMEOUT — push stuck LOW (not releasing)");
    pushResults[index] = FAIL;
    return;
  }

  Serial.println("  ↑ RELEASED");
  Serial.println("  ✓ PASS");
  pushResults[index] = PASS;
}

// --- Phase 3: Encoders ---

void testEncoder(int index) {
  int jHeader = index + 1;
  int pinA = index * 2;
  int pinB = index * 2 + 1;

  printDivider();
  Serial.print("Encoder ");
  Serial.print(index + 1);
  Serial.print(" rotation — J");
  Serial.print(jHeader);
  Serial.print(" (pins ");
  Serial.print(pinA);
  Serial.print("/");
  Serial.print(pinB);
  Serial.println(")");

  // Reset all encoders to zero
  for (int i = 0; i < 8; i++) encoders[i]->write(0);

  // Test CW
  Serial.println("  Turn CLOCKWISE (several clicks). (10s timeout)");
  encoderCW[index] = false;
  unsigned long start = millis();
  while (millis() - start < TEST_TIMEOUT) {
    long pos = encoders[index]->read();
    if (pos >= 4) { // at least one detent CW
      encoderCW[index] = true;
      Serial.print("  → CW detected (position: ");
      Serial.print(pos);
      Serial.println(")");
      checkSpuriousEncoders(index);
      break;
    }
    if (pos <= -4) {
      Serial.print("  ⚠ CCW detected instead of CW (position: ");
      Serial.print(pos);
      Serial.println(") — A/B channels may be swapped");
      encoderCW[index] = true; // still responding, just inverted
      checkSpuriousEncoders(index);
      break;
    }
  }
  if (!encoderCW[index]) {
    Serial.println("  ✗ TIMEOUT — no rotation detected");
  }

  delay(500);

  // Reset and test CCW
  for (int i = 0; i < 8; i++) encoders[i]->write(0);

  Serial.println("  Turn COUNTER-CLOCKWISE (several clicks). (10s timeout)");
  encoderCCW[index] = false;
  start = millis();
  while (millis() - start < TEST_TIMEOUT) {
    long pos = encoders[index]->read();
    if (pos <= -4) { // at least one detent CCW
      encoderCCW[index] = true;
      Serial.print("  ← CCW detected (position: ");
      Serial.print(pos);
      Serial.println(")");
      checkSpuriousEncoders(index);
      break;
    }
    if (pos >= 4) {
      Serial.print("  ⚠ CW detected instead of CCW (position: ");
      Serial.print(pos);
      Serial.println(") — A/B channels may be swapped");
      encoderCCW[index] = true;
      checkSpuriousEncoders(index);
      break;
    }
  }
  if (!encoderCCW[index]) {
    Serial.println("  ✗ TIMEOUT — no rotation detected");
  }

  // Determine result
  if (encoderCW[index] && encoderCCW[index]) {
    Serial.println("  ✓ PASS");
    encoderResults[index] = PASS;
  } else if (encoderCW[index] || encoderCCW[index]) {
    Serial.println("  ~ PARTIAL — one direction failed");
    encoderResults[index] = PARTIAL;
  } else {
    Serial.println("  ✗ FAIL — no rotation in either direction");
    encoderResults[index] = FAIL;
  }
}

// --- Summary ---

void printSummary() {
  printHeader("TEST SUMMARY");

  const char* resultStr[] = {"UNTESTED", "PASS", "FAIL", "PARTIAL"};
  int failures = 0;

  Serial.println("Buttons (J9-J16):");
  for (int i = 0; i < 8; i++) {
    Serial.print("  Button ");
    Serial.print(i + 1);
    Serial.print(" (J");
    Serial.print(i + 9);
    Serial.print("): ");
    Serial.println(resultStr[buttonResults[i]]);
    if (buttonResults[i] != PASS) failures++;
  }
  Serial.println();

  Serial.println("Encoder pushes (J1-J8):");
  for (int i = 0; i < 8; i++) {
    Serial.print("  Encoder ");
    Serial.print(i + 1);
    Serial.print(" push (J");
    Serial.print(i + 1);
    Serial.print("): ");
    Serial.println(resultStr[pushResults[i]]);
    if (pushResults[i] != PASS) failures++;
  }
  Serial.println();

  Serial.println("Encoder rotation (J1-J8):");
  for (int i = 0; i < 8; i++) {
    Serial.print("  Encoder ");
    Serial.print(i + 1);
    Serial.print(" (J");
    Serial.print(i + 1);
    Serial.print("): ");
    Serial.print(resultStr[encoderResults[i]]);
    if (encoderResults[i] == PARTIAL) {
      Serial.print(" — CW:");
      Serial.print(encoderCW[i] ? "ok" : "FAIL");
      Serial.print(" CCW:");
      Serial.print(encoderCCW[i] ? "ok" : "FAIL");
    }
    Serial.println();
    if (encoderResults[i] != PASS) failures++;
  }
  Serial.println();

  if (spuriousButtonCount || spuriousPushCount || spuriousEncoderCount) {
    Serial.println("Spurious triggers detected:");
    if (spuriousButtonCount) {
      Serial.print("  Buttons: ");
      Serial.print(spuriousButtonCount);
      Serial.println(" events");
    }
    if (spuriousPushCount) {
      Serial.print("  Encoder pushes: ");
      Serial.print(spuriousPushCount);
      Serial.println(" events");
    }
    if (spuriousEncoderCount) {
      Serial.print("  Encoder rotation: ");
      Serial.print(spuriousEncoderCount);
      Serial.println(" events");
    }
    Serial.println();
  }

  printDivider();
  if (failures == 0 && spuriousButtonCount == 0 && spuriousPushCount == 0 && spuriousEncoderCount == 0) {
    Serial.println("ALL 24 INPUTS PASSED — no issues detected.");
  } else {
    Serial.print(failures);
    Serial.println(" input(s) failed or partial.");
    if (failures > 0) {
      Serial.println();
      Serial.println("Debugging guide:");
      Serial.println("  Single button fail  → check loom crimp on that wire");
      Serial.println("  All buttons fail    → check common ground on button looms");
      Serial.println("  Encoder no rotation → check A/B wires on that loom");
      Serial.println("  Encoder one dir     → one channel (A or B) not connected");
      Serial.println("  Push fail           → check switch wire on encoder loom");
      Serial.println("  Spurious triggers   → short between adjacent pins/traces");
      Serial.println();
      Serial.println("To isolate loom vs socket: move a failing loom to a");
      Serial.println("known-good socket. If it still fails, loom is the problem.");
    }
  }
  Serial.println();
  Serial.println("Test complete. Reset Teensy to run again.");
}

// --- Main ---

void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(PUSH_PINS[i], INPUT_PULLUP);
    buttonResults[i] = UNTESTED;
    pushResults[i] = UNTESTED;
    encoderResults[i] = UNTESTED;
    encoderCW[i] = false;
    encoderCCW[i] = false;
  }

  Serial.println("╔══════════════════════════════════════════════════╗");
  Serial.println("║       SYSTEMATIC INPUT DIAGNOSTIC TEST           ║");
  Serial.println("╠══════════════════════════════════════════════════╣");
  Serial.println("║ Tests all 24 inputs one at a time:               ║");
  Serial.println("║   Phase 1: 8 buttons        (J9-J16)            ║");
  Serial.println("║   Phase 2: 8 encoder pushes  (J1-J8)            ║");
  Serial.println("║   Phase 3: 8 encoder turns   (J1-J8)            ║");
  Serial.println("║                                                  ║");
  Serial.println("║ Each input has a 10-second timeout.              ║");
  Serial.println("║ If you can't test one, just wait for timeout.    ║");
  Serial.println("║ Spurious triggers on other inputs are flagged.   ║");
  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();

  // --- Phase 1: Buttons ---
  printHeader("PHASE 1: BUTTONS (J9-J16)");
  Serial.println("For each button, press and release when prompted.");
  Serial.println();

  for (int i = 0; i < 8; i++) {
    testButton(i);
  }

  // --- Phase 2: Encoder pushes ---
  printHeader("PHASE 2: ENCODER PUSH SWITCHES (J1-J8)");
  Serial.println("For each encoder, push the shaft down and release.");
  Serial.println();

  for (int i = 0; i < 8; i++) {
    testPush(i);
  }

  // --- Phase 3: Encoder rotation ---
  printHeader("PHASE 3: ENCODER ROTATION (J1-J8)");
  Serial.println("For each encoder, turn CW then CCW when prompted.");
  Serial.println();

  for (int i = 0; i < 8; i++) {
    testEncoder(i);
  }

  // --- Summary ---
  printSummary();
}

void loop() {
  // Nothing — test runs once in setup
}
