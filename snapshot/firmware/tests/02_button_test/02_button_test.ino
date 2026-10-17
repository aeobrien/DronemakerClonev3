// Test 2: Button Test
// Purpose: Verify button switch inputs on pins 24-31.
// Connect one or more buttons via JST cables to J9-J16.
// Open serial monitor — pressing a button should print its number.
//
// Expected: "Button X PRESSED" when pressed, "Button X RELEASED" when released.
// If you get garbage or constant triggering, check: wiring, JST orientation, solder joints.

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int NUM_BUTTONS = 8;

bool lastState[NUM_BUTTONS];

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastState[i] = HIGH; // unpressed
  }

  Serial.println("=== Button Test ===");
  Serial.println("Press any button (J9-J16). You should see PRESSED/RELEASED messages.");
  Serial.println("If nothing happens, check JST cable orientation and solder joints.");
  Serial.println();
}

void loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool state = digitalRead(BUTTON_PINS[i]);

    if (state != lastState[i]) {
      delay(5); // simple debounce
      state = digitalRead(BUTTON_PINS[i]);

      if (state != lastState[i]) {
        Serial.print("Button ");
        Serial.print(i + 1);
        Serial.print(" (pin ");
        Serial.print(BUTTON_PINS[i]);
        Serial.print("): ");
        Serial.println(state == LOW ? "PRESSED" : "RELEASED");
        lastState[i] = state;
      }
    }
  }
}
