// Test 4c: LED Bit Mapper
// Purpose: Walk through all 24 shift register bits one at a time.
// Each step lights exactly ONE LED output. You tell the serial monitor
// what you see, and we build the mapping.
//
// Press any button to advance to the next bit.
// Serial monitor tells you which byte and bit is active.
// You write down which button lights up and what colour.

const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int NUM_BUTTONS = 8;

bool lastButtonState = HIGH;
int currentBit = -1; // start at -1, first press goes to 0

void shiftOutRaw(byte b1, byte b2, byte b3) {
  // b1 shifts out first (farthest in chain = U3)
  // b2 shifts out second (middle = U2)
  // b3 shifts out last (nearest = U1)
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

void lightBit(int bitIndex) {
  // bitIndex 0-7:   byte 3 (shifted last,  nearest chip U1)
  // bitIndex 8-15:  byte 2 (shifted second, middle chip U2)
  // bitIndex 16-23: byte 1 (shifted first,  farthest chip U3)

  byte b1 = 0, b2 = 0, b3 = 0;

  if (bitIndex < 8) {
    b3 = 1 << bitIndex;        // U1
  } else if (bitIndex < 16) {
    b2 = 1 << (bitIndex - 8);  // U2
  } else {
    b1 = 1 << (bitIndex - 16); // U3
  }

  // Active-low LEDs: invert so our target bit turns ON, everything else OFF
  shiftOutRaw(~b1, ~b2, ~b3);

  int chipNum = (bitIndex / 8) + 1;
  int chipBit = bitIndex % 8;

  Serial.print(">>> BIT ");
  Serial.print(bitIndex);
  Serial.print(" — Chip U");
  Serial.print(chipNum);
  Serial.print(", output Q");
  Serial.print(chipBit);
  Serial.println(" is ON. Everything else OFF.");
  Serial.println("    What do you see? Which button number, and what colour?");
  Serial.println("    Press any button to advance to next bit.");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  // All LEDs off
  shiftOutRaw(0xFF, 0xFF, 0xFF); // all high = all off (active-low)

  Serial.println("=== LED Bit Mapper ===");
  Serial.println("This walks through all 24 shift register outputs one by one.");
  Serial.println("Press any button to start, then press again to advance.");
  Serial.println("For each step, note which button lights up and what colour.");
  Serial.println();
}

void loop() {
  // Check if ANY button is pressed
  bool anyPressed = false;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(BUTTON_PINS[i]) == LOW) {
      anyPressed = true;
      break;
    }
  }

  if (anyPressed && lastButtonState == HIGH) {
    delay(50); // debounce

    // Confirm still pressed after debounce
    bool stillPressed = false;
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        stillPressed = true;
        break;
      }
    }
    if (!stillPressed) {
      lastButtonState = HIGH;
      return;
    }

    currentBit++;

    if (currentBit >= 24) {
      Serial.println("=== ALL 24 BITS TESTED ===");
      Serial.println("All done! Use the mapping to fix the firmware.");
      shiftOutRaw(0xFF, 0xFF, 0xFF); // all off
      currentBit = 24; // stay here
    } else {
      lightBit(currentBit);
    }

    // Wait for button release before allowing next advance
    while (true) {
      bool anyHeld = false;
      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(BUTTON_PINS[i]) == LOW) {
          anyHeld = true;
          break;
        }
      }
      if (!anyHeld) break;
      delay(10);
    }
    delay(50); // release debounce
  }

  lastButtonState = anyPressed ? LOW : HIGH;
}
