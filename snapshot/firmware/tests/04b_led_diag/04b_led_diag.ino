// Test 4b: LED Diagnostic
// Purpose: Press a button to cycle its LED through: OFF → RED → GREEN → BLUE → WHITE → OFF
// Each button press advances that button's colour by one step.
// Serial monitor shows exactly which shift register bits are being set.
//
// This lets you check each button/socket individually and see if the colour
// you expect matches what you actually see.

const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int NUM_BUTTONS = 8;

// Colour cycle: OFF, RED, GREEN, BLUE, WHITE
const int NUM_COLOURS = 5;
const char* COLOUR_NAMES[] = {"OFF", "RED", "GREEN", "BLUE", "WHITE"};

// Which RGB bits are on for each colour (1 = on)
const bool COLOUR_R[] = {false, true,  false, false, true};
const bool COLOUR_G[] = {false, false, true,  false, true};
const bool COLOUR_B[] = {false, false, false, true,  true};

int buttonColour[NUM_BUTTONS]; // current colour index per button
bool lastState[NUM_BUTTONS];

// Current shift register state
byte redByte   = 0x00;
byte greenByte = 0x00;
byte blueByte  = 0x00;

void updateLEDs() {
  // Active-low: 0 = on, 1 = off. Inversion done here.
  // Shift order: red first (farthest), blue second, green last (nearest).
  byte b1 = (byte)(~redByte);    // farthest chip
  byte b2 = (byte)(~blueByte);   // middle chip
  byte b3 = (byte)(~greenByte);  // nearest chip
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

void printState() {
  Serial.println("--- Current state ---");
  for (int i = 0; i < NUM_BUTTONS; i++) {
    byte mask = 1 << i;
    Serial.print("  Button ");
    Serial.print(i + 1);
    Serial.print(" (J");
    Serial.print(i + 9);
    Serial.print("): ");
    Serial.print(COLOUR_NAMES[buttonColour[i]]);
    Serial.print("  [R=");
    Serial.print((redByte & mask) ? "ON" : "off");
    Serial.print(" G=");
    Serial.print((greenByte & mask) ? "ON" : "off");
    Serial.print(" B=");
    Serial.print((blueByte & mask) ? "ON" : "off");
    Serial.println("]");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastState[i] = HIGH;
    buttonColour[i] = 0; // start OFF
  }

  // All LEDs off
  updateLEDs();

  Serial.println("=== LED Diagnostic ===");
  Serial.println("Press each button to cycle its LED: OFF → RED → GREEN → BLUE → WHITE → OFF");
  Serial.println("Compare what you SEE with what the serial monitor SAYS it should be.");
  Serial.println("If they don't match, the wiring for that colour/socket is wrong.");
  Serial.println();
  printState();
}

void loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool state = digitalRead(BUTTON_PINS[i]);

    if (state == LOW && lastState[i] == HIGH) {
      delay(20); // debounce
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        // Advance colour
        buttonColour[i] = (buttonColour[i] + 1) % NUM_COLOURS;
        int c = buttonColour[i];
        byte mask = 1 << (i + 1);  // Buttons map to bits 1-8 (bit 0 unused)

        // Update the shift register bytes
        if (COLOUR_R[c]) redByte   |= mask; else redByte   &= ~mask;
        if (COLOUR_G[c]) greenByte |= mask; else greenByte &= ~mask;
        if (COLOUR_B[c]) blueByte  |= mask; else blueByte  &= ~mask;

        updateLEDs();

        Serial.print("Button ");
        Serial.print(i + 1);
        Serial.print(" (J");
        Serial.print(i + 9);
        Serial.print(") → ");
        Serial.print(COLOUR_NAMES[c]);
        Serial.print("  [R=");
        Serial.print(COLOUR_R[c] ? "ON" : "off");
        Serial.print(" G=");
        Serial.print(COLOUR_G[c] ? "ON" : "off");
        Serial.print(" B=");
        Serial.print(COLOUR_B[c] ? "ON" : "off");
        Serial.println("]");
      }
    }
    lastState[i] = state;
  }
}
