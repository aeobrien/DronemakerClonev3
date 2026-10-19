// Test 4d: Simplest possible LED test — NO INVERSION HELPERS
// Raw bytes only. Active-low means: 0 = LED on, 1 = LED off.
// We construct the exact byte to send — no ~ operator anywhere.

const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
int phase = -1;

void sendRaw(byte b1, byte b2, byte b3) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

void waitForPress() {
  while (true) {
    bool any = false;
    for (int i = 0; i < 8; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) { any = true; break; }
    }
    if (any) break;
    delay(10);
  }
  delay(50);
  while (true) {
    bool any = false;
    for (int i = 0; i < 8; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) { any = true; break; }
    }
    if (!any) break;
    delay(10);
  }
  delay(50);
}

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  // All off: send 0xFF (all high = all off)
  sendRaw(0xFF, 0xFF, 0xFF);

  Serial.println("=== Simple LED Test (no inversion) ===");
  Serial.println("Press any button to advance.");
  Serial.println();
}

void loop() {
  waitForPress();
  phase++;

  // Active-low: 0x00 = all 8 bits ON, 0xFF = all OFF
  // 0xFD = bit 1 ON (0xFD = 11111101), rest off
  // 0xFB = bit 2 ON (0xFB = 11111011), rest off

  switch (phase) {
    case 0:
      Serial.println("Phase 0: ALL OFF");
      sendRaw(0xFF, 0xFF, 0xFF);
      Serial.println("  -> Everything should be off");
      break;

    case 1:
      Serial.println("Phase 1: Byte 3 ALL ON (0x00)");
      sendRaw(0xFF, 0xFF, 0x00);
      Serial.println("  -> What colour? Which buttons?");
      break;

    case 2:
      Serial.println("Phase 2: Byte 2 ALL ON (0x00)");
      sendRaw(0xFF, 0x00, 0xFF);
      Serial.println("  -> What colour? Which buttons?");
      break;

    case 3:
      Serial.println("Phase 3: Byte 1 ALL ON (0x00)");
      sendRaw(0x00, 0xFF, 0xFF);
      Serial.println("  -> What colour? Which buttons?");
      break;

    case 4:
      Serial.println("Phase 4: ALL THREE ON (0x00, 0x00, 0x00)");
      sendRaw(0x00, 0x00, 0x00);
      Serial.println("  -> Should be white on all buttons");
      break;

    case 5:
      Serial.println("Phase 5: Byte 3, bit 1 ON (send 0xFD)");
      sendRaw(0xFF, 0xFF, 0xFD);
      Serial.println("  -> One button, one colour. Which?");
      break;

    case 6:
      Serial.println("Phase 6: Byte 3, bit 2 ON (send 0xFB)");
      sendRaw(0xFF, 0xFF, 0xFB);
      Serial.println("  -> One button, one colour. Which?");
      break;

    case 7:
      Serial.println("Phase 7: Byte 2, bit 1 ON (send 0xFD)");
      sendRaw(0xFF, 0xFD, 0xFF);
      Serial.println("  -> One button, different colour. Which?");
      break;

    case 8:
      Serial.println("Phase 8: Byte 1, bit 1 ON (send 0xFD)");
      sendRaw(0xFD, 0xFF, 0xFF);
      Serial.println("  -> One button, third colour. Which?");
      break;

    case 9:
      Serial.println("Phase 9: ALL OFF");
      sendRaw(0xFF, 0xFF, 0xFF);
      Serial.println("=== DONE ===");
      break;
  }
}
