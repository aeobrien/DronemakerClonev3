// Test 4: LED Test — corrected mapping
// Cycles through: all red → all green → all blue → all white → all off → one-at-a-time.
// Connect buttons via JST to J9-J16 to see their LEDs.
//
// Mapping from 04c test:
//   Green = bits 1-8 in the LAST byte shifted out (nearest chip)
//   Blue  = bits 1-8 in the MIDDLE byte
//   Red   = bits 1-8 in the FIRST byte shifted out (farthest chip)

const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;

// Send raw bytes to shift registers (already inverted for active-low).
// b1 shifted first (farthest chip), b3 shifted last (nearest chip).
void shiftOutRaw(byte b1, byte b2, byte b3) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

// Set LEDs by colour name. Handles active-low inversion.
// From mapping: red = first out (farthest), blue = middle, green = last out (nearest)
void setLEDs(byte red, byte green, byte blue) {
  byte b1 = (byte)(~red);
  byte b2 = (byte)(~blue);
  byte b3 = (byte)(~green);
  shiftOutRaw(b1, b2, b3);
}

void allOff() {
  shiftOutRaw(0xFF, 0xFF, 0xFF);  // all high = all off (active-low)
}

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  allOff();

  Serial.println("=== LED Test (corrected mapping) ===");
  Serial.println("Sequence: RED -> GREEN -> BLUE -> WHITE -> OFF -> walking");
  Serial.println();
}

void loop() {
  const byte ALL = 0xFE;  // bits 1-7

  // All red
  Serial.println("All RED");
  setLEDs(ALL, 0x00, 0x00);
  delay(1500);

  // All green
  Serial.println("All GREEN");
  setLEDs(0x00, ALL, 0x00);
  delay(1500);

  // All blue
  Serial.println("All BLUE");
  setLEDs(0x00, 0x00, ALL);
  delay(1500);

  // All white
  Serial.println("All WHITE");
  setLEDs(ALL, ALL, ALL);
  delay(1500);

  // All off
  Serial.println("All OFF");
  allOff();
  delay(1000);

  // Walking green
  Serial.println("Walking green (bits 1-8)...");
  for (int i = 0; i < 8; i++) {
    byte mask = 1 << (i + 1);
    setLEDs(0x00, mask, 0x00);
    Serial.print("  Button ");
    Serial.print(i + 1);
    Serial.println(" GREEN");
    delay(400);
  }
  allOff();
  delay(500);

  // Walking red
  Serial.println("Walking red (bits 1-8)...");
  for (int i = 0; i < 8; i++) {
    byte mask = 1 << (i + 1);
    setLEDs(mask, 0x00, 0x00);
    delay(400);
  }
  allOff();
  delay(500);

  // Walking blue
  Serial.println("Walking blue (bits 1-8)...");
  for (int i = 0; i < 8; i++) {
    byte mask = 1 << (i + 1);
    setLEDs(0x00, 0x00, mask);
    delay(400);
  }
  allOff();
  delay(1000);

  // Now try bits 0-7 in case the offset is wrong
  Serial.println("Walking green (bits 0-7)...");
  for (int i = 0; i < 8; i++) {
    byte mask = 1 << i;
    setLEDs(0x00, mask, 0x00);
    Serial.print("  Button ");
    Serial.print(i + 1);
    Serial.println(" GREEN");
    delay(400);
  }
  allOff();
  delay(2000);
}
