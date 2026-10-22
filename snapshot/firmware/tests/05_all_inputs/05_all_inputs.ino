// Test 5: All Inputs Combined
// Purpose: Verify all 8 encoders, 8 encoder pushes, and 8 buttons work together.
// Also: pressing a button lights its LED green. Turning an encoder adjusts its
// button's colour (hue cycles through the rainbow). This confirms the full
// input → processing → output chain.
//
// Requires: Encoder library

#include <Encoder.h>

// --- Pin assignments ---
const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int PUSH_PINS[]   = {16, 17, 18, 19, 20, 21, 22, 23};
const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // Swapped — PCB layout error on U1/U3 pins 11/12
const int LATCH_PIN = 33;
const int NUM_CHANNELS = 8;

// --- Encoders ---
Encoder enc1(1, 0);   // A/B swapped — loom wiring has channels reversed
Encoder enc2(3, 2);
Encoder enc3(5, 4);
Encoder enc4(7, 6);
Encoder enc5(9, 8);
Encoder enc6(11, 10);
Encoder enc7(13, 12);
Encoder enc8(15, 14);
Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7, &enc8};

// --- State ---
long lastPosition[NUM_CHANNELS];
bool lastButton[NUM_CHANNELS];
bool lastPush[NUM_CHANNELS];
bool buttonLit[NUM_CHANNELS];       // toggle: is this button's LED on?
int encoderColour[NUM_CHANNELS];    // 0-6: which colour to show (cycled by encoder)

byte ledGreen = 0, ledRed = 0, ledBlue = 0;

void shiftOut24(byte blue, byte red, byte green) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, ~blue);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, ~red);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, ~green);
  digitalWrite(LATCH_PIN, HIGH);
}

void updateLEDs() {
  ledRed = 0; ledGreen = 0; ledBlue = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (!buttonLit[i]) continue;
    byte mask = 1 << i;
    switch (encoderColour[i]) {
      case 0: ledRed   |= mask; break;                          // red
      case 1: ledGreen |= mask; break;                          // green
      case 2: ledBlue  |= mask; break;                          // blue
      case 3: ledRed   |= mask; ledGreen |= mask; break;       // yellow
      case 4: ledGreen |= mask; ledBlue  |= mask; break;       // cyan
      case 5: ledRed   |= mask; ledBlue  |= mask; break;       // magenta
      case 6: ledRed   |= mask; ledGreen |= mask; ledBlue |= mask; break; // white
    }
  }
  shiftOut24(ledBlue, ledRed, ledGreen);
}

void setup() {
  Serial.begin(115200);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(PUSH_PINS[i], INPUT_PULLUP);
    lastButton[i] = HIGH;
    lastPush[i] = HIGH;
    lastPosition[i] = 0;
    buttonLit[i] = false;
    encoderColour[i] = 1; // start green
  }

  shiftOut24(0, 0, 0);

  Serial.println("=== All Inputs Test ===");
  Serial.println("Press a button to toggle its LED on/off.");
  Serial.println("Turn its encoder to cycle the LED colour.");
  Serial.println("Push an encoder shaft to reset that channel.");
  Serial.println();
}

void loop() {
  // Buttons — toggle LED on/off
  for (int i = 0; i < NUM_CHANNELS; i++) {
    bool state = digitalRead(BUTTON_PINS[i]);
    if (state != lastButton[i]) {
      delay(5);
      state = digitalRead(BUTTON_PINS[i]);
      if (state != lastButton[i]) {
        if (state == LOW) {
          buttonLit[i] = !buttonLit[i];
          Serial.print("Button ");
          Serial.print(i + 1);
          Serial.println(buttonLit[i] ? ": ON" : ": OFF");
          updateLEDs();
        }
        lastButton[i] = state;
      }
    }
  }

  // Encoder push — reset channel
  for (int i = 0; i < NUM_CHANNELS; i++) {
    bool state = digitalRead(PUSH_PINS[i]);
    if (state != lastPush[i]) {
      delay(5);
      state = digitalRead(PUSH_PINS[i]);
      if (state != lastPush[i]) {
        if (state == LOW) {
          buttonLit[i] = false;
          encoderColour[i] = 1;
          encoders[i]->write(0);
          lastPosition[i] = 0;
          Serial.print("Encoder ");
          Serial.print(i + 1);
          Serial.println(" push: RESET");
          updateLEDs();
        }
        lastPush[i] = state;
      }
    }
  }

  // Encoders — cycle colour
  for (int i = 0; i < NUM_CHANNELS; i++) {
    long pos = encoders[i]->read();
    if (pos != lastPosition[i]) {
      int delta = pos - lastPosition[i];
      if (abs(delta) >= 4) { // one detent = 4 counts on PEC16
        int steps = delta / 4;
        encoderColour[i] = (encoderColour[i] + steps + 7) % 7;

        const char* colourNames[] = {"RED", "GREEN", "BLUE", "YELLOW", "CYAN", "MAGENTA", "WHITE"};
        Serial.print("Encoder ");
        Serial.print(i + 1);
        Serial.print(": colour = ");
        Serial.println(colourNames[encoderColour[i]]);

        lastPosition[i] = pos;
        updateLEDs();
      }
    }
  }
}
