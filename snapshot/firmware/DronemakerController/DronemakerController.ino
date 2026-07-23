/*
 * DronemakerClone v3 — MIDI Controller Firmware
 * ===============================================
 *
 * Hardware: Teensy 4.1
 * Arduino IDE USB Type: "Serial + MIDI"
 *
 * Physical controls:
 *   - 8 rotary encoders (PEC16-4220F-S0024) with push switches
 *   - 8 illuminated RGB momentary push buttons (Adafruit 3350)
 *   - 3 daisy-chained 74HC595 shift registers driving button LEDs
 *
 * Pin assignments:
 *   Encoders A/B:     0/1, 2/3, 4/5, 6/7, 8/9, 10/11, 12/13, 14/15
 *   Encoder pushes:   16, 17, 18, 19, 20, 21, 22, 23  (INPUT_PULLUP, active-low)
 *   Button switches:  24, 25, 26, 27, 28, 29, 30, 31  (INPUT_PULLUP, active-low)
 *   Shift registers:  32 (DATA/SER), 34 (CLOCK/SRCLK), 33 (LATCH/RCLK)
 *                     NOTE: pins 33/34 swapped vs original design due to PCB
 *                     layout error (SRCLK/RCLK transposed on U1 and U3).
 *                     U2 has physical pin swap to compensate.
 *
 * Shift register chain (active-low LED logic — shift out inverted bytes):
 *   Byte 1 (first out) -> farthest chip = red
 *   Byte 2             -> middle chip  = blue
 *   Byte 3 (last out)  -> nearest chip = green
 *
 * Bit mapping: button N (0-7) maps to bit N+1 in each byte.
 * Bit 0 of each byte is unused (not connected to any socket).
 *
 * MIDI mapping (Channel 1):
 *   Encoders:         CC 20-27, relative mode (65 = CW, 63 = CCW)
 *   Encoder pushes:   CC 102-109 (127 on press, 0 on release)
 *   Buttons:          Note 36-43 (velocity 127 on press, Note Off on release)
 *   Button toggles:   Note 44-51 (toggle — press = Note On 127, press again = Note Off)
 *
 * Incoming MIDI (from Pi for LED control):
 *   CC 40-47:  Set LED colour for buttons 1-8
 *              Value: 0=off, 1=red, 2=green, 3=yellow, 4=blue,
 *                     5=magenta, 6=cyan, 7=white
 *
 * Dependencies:
 *   - Encoder library by Paul Stoffregen (install via Library Manager)
 *   - Teensy MIDI (built into Teensyduino)
 *
 * Build instructions:
 *   1. In Arduino IDE: Tools > Board > Teensy 4.1
 *   2. Tools > USB Type > "Serial + MIDI"
 *   3. Install Encoder library via Library Manager
 *   4. Upload
 */

#include <Encoder.h>

// =============================================================================
// Pin Assignments
// =============================================================================

const int BUTTON_PINS[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int PUSH_PINS[]   = {16, 17, 18, 19, 20, 21, 22, 23};
const int DATA_PIN  = 32;
const int CLOCK_PIN = 34;  // PCB routes pin 34 to SRCLK on U1/U3 (pin 11/12 swap in layout)
const int LATCH_PIN = 33;  // PCB routes pin 33 to RCLK on U1/U3 — U2 has pins physically swapped

const int NUM_CHANNELS = 8;

// =============================================================================
// MIDI Configuration
// =============================================================================

const int MIDI_CHANNEL = 1;

// Encoder CCs (relative mode)
const int ENCODER_CC_BASE = 20;       // CC 20-27
const int ENCODER_CW      = 65;      // Relative: clockwise
const int ENCODER_CCW      = 63;     // Relative: counter-clockwise

// Encoder push CCs
const int ENCODER_PUSH_CC_BASE = 102; // CC 102-109

// Button notes
const int BUTTON_NOTE_BASE  = 36;     // Notes 36-43 (momentary)
const int BUTTON_TOGGLE_BASE = 44;    // Notes 44-51 (toggle)

// Incoming LED control CCs
const int LED_CC_BASE = 40;           // CC 40-47

// Velocity / CC values
const int VELOCITY_ON  = 127;
const int VELOCITY_OFF = 0;

// =============================================================================
// 14-bit MIDI CC Support (prepared, not active)
// =============================================================================
//
// For 14-bit CC, use CC pairs: MSB on CC 20-27, LSB on CC 52-59 (MSB + 32).
// Uncomment the following and modify the encoder handler to split a 14-bit
// value into MSB/LSB pairs. Not needed for relative mode.
//
// const int ENCODER_CC_LSB_BASE = 52;  // CC 52-59 (LSB for 14-bit)
//
// void sendCC14(int channel, int ccMSB, int value14) {
//   int msb = (value14 >> 7) & 0x7F;
//   int lsb = value14 & 0x7F;
//   usbMIDI.sendControlChange(ccMSB, msb, channel);
//   usbMIDI.sendControlChange(ccMSB + 32, lsb, channel);
// }

// =============================================================================
// Encoders
// =============================================================================

Encoder enc1(1, 0);   // A/B swapped — loom wiring has channels reversed
Encoder enc2(3, 2);
Encoder enc3(5, 4);
Encoder enc4(7, 6);
Encoder enc5(9, 8);
Encoder enc6(11, 10);
Encoder enc7(13, 12);
Encoder enc8(15, 14);

Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7, &enc8};

// =============================================================================
// Debouncing
// =============================================================================

const unsigned long DEBOUNCE_MS = 5;

// =============================================================================
// State
// =============================================================================

// Encoder state
long encoderPosition[NUM_CHANNELS];

// Encoder push switch state
bool pushState[NUM_CHANNELS];
unsigned long pushDebounceTime[NUM_CHANNELS];

// Button switch state
bool buttonState[NUM_CHANNELS];
unsigned long buttonDebounceTime[NUM_CHANNELS];

// Button toggle state (for Note 44-51 toggle behaviour)
bool buttonToggleOn[NUM_CHANNELS];

// LED colour state (0-7, set by incoming MIDI CC 40-47)
byte ledColour[NUM_CHANNELS];

// Shift register buffers
byte ledRed   = 0;
byte ledGreen = 0;
byte ledBlue  = 0;

// =============================================================================
// Shift Register Output
// =============================================================================

void shiftOut24(byte red, byte blue, byte green) {
  // Shift order: red first (farthest chip), blue second, green last (nearest chip)
  // Mapping: green = bits 1-8 in nearest chip, blue = middle, red = farthest
  // Active-low: invert all bytes (0 = LED on, 1 = LED off)
  byte b1 = (byte)(~red);
  byte b2 = (byte)(~blue);
  byte b3 = (byte)(~green);
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, b3);
  digitalWrite(LATCH_PIN, HIGH);
}

// =============================================================================
// LED Colour Management
// =============================================================================
//
// Colour encoding (matches incoming MIDI CC values):
//   0 = off
//   1 = red
//   2 = green
//   3 = yellow  (red + green)
//   4 = blue
//   5 = magenta (red + blue)
//   6 = cyan    (green + blue)
//   7 = white   (red + green + blue)
//
// Bit layout: value = 0bBGR (bit 2 = blue, bit 1 = green, bit 0 = red)

void updateLEDs() {
  ledRed   = 0;
  ledGreen = 0;
  ledBlue  = 0;

  for (int i = 0; i < NUM_CHANNELS; i++) {
    byte colour = ledColour[i];
    if (colour == 0) continue;

    byte mask = 1 << (i + 1);  // Buttons map to bits 1-8 (bit 0 unused)
    if (colour & 0x01) ledRed   |= mask;  // bit 0 = red
    if (colour & 0x02) ledGreen |= mask;  // bit 1 = green
    if (colour & 0x04) ledBlue  |= mask;  // bit 2 = blue
  }

  shiftOut24(ledRed, ledBlue, ledGreen);
}

// =============================================================================
// Incoming MIDI Handler
// =============================================================================

void handleControlChange(byte channel, byte control, byte value) {
  // LED colour control: CC 40-47
  if (control >= LED_CC_BASE && control < LED_CC_BASE + NUM_CHANNELS) {
    int index = control - LED_CC_BASE;
    ledColour[index] = value & 0x07;  // Clamp to 0-7
    updateLEDs();
  }
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
  Serial.begin(115200);

  // Shift register pins
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  // Initialise all state
  for (int i = 0; i < NUM_CHANNELS; i++) {
    // Switch pins
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(PUSH_PINS[i], INPUT_PULLUP);

    // Encoder state
    encoderPosition[i] = 0;

    // Push switch state
    pushState[i] = HIGH;  // Unpressed (active-low)
    pushDebounceTime[i] = 0;

    // Button state
    buttonState[i] = HIGH;  // Unpressed (active-low)
    buttonDebounceTime[i] = 0;

    // Toggle state
    buttonToggleOn[i] = false;

    // LED state
    ledColour[i] = 0;  // Off
  }

  // All LEDs off
  shiftOut24(0, 0, 0);

  // Register incoming MIDI handler
  usbMIDI.setHandleControlChange(handleControlChange);

  Serial.println("DronemakerClone v3 MIDI Controller");
  Serial.println("Ready. USB Type must be 'Serial + MIDI'.");
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
  unsigned long now = millis();

  // -------------------------------------------------------------------------
  // Read incoming MIDI (Pi -> Teensy LED control)
  // -------------------------------------------------------------------------
  // usbMIDI.read() dispatches to registered handlers (handleControlChange)
  usbMIDI.read();

  // -------------------------------------------------------------------------
  // Encoders — relative CC mode (CC 20-27)
  // -------------------------------------------------------------------------
  // PEC16 encoders produce 4 counts per detent. We accumulate raw counts
  // and emit one MIDI message per detent crossing.

  for (int i = 0; i < NUM_CHANNELS; i++) {
    long pos = encoders[i]->read();
    long delta = pos - encoderPosition[i];

    if (abs(delta) >= 4) {
      // Calculate number of whole detents
      int detents = delta / 4;

      // Send one CC per detent step
      for (int d = 0; d < abs(detents); d++) {
        usbMIDI.sendControlChange(
          ENCODER_CC_BASE + i,
          detents > 0 ? ENCODER_CW : ENCODER_CCW,
          MIDI_CHANNEL
        );
      }

      // Snap position to detent boundary (preserve remainder)
      encoderPosition[i] += detents * 4;
    }
  }

  // -------------------------------------------------------------------------
  // Encoder Push Switches — CC 102-109 (momentary)
  // -------------------------------------------------------------------------

  for (int i = 0; i < NUM_CHANNELS; i++) {
    bool reading = digitalRead(PUSH_PINS[i]);

    if (reading != pushState[i]) {
      if (now - pushDebounceTime[i] >= DEBOUNCE_MS) {
        pushState[i] = reading;
        pushDebounceTime[i] = now;

        if (reading == LOW) {
          // Pressed
          usbMIDI.sendControlChange(
            ENCODER_PUSH_CC_BASE + i,
            VELOCITY_ON,
            MIDI_CHANNEL
          );
        } else {
          // Released
          usbMIDI.sendControlChange(
            ENCODER_PUSH_CC_BASE + i,
            VELOCITY_OFF,
            MIDI_CHANNEL
          );
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  // Button Switches — Note 36-43 (momentary) + Note 44-51 (toggle)
  // -------------------------------------------------------------------------

  for (int i = 0; i < NUM_CHANNELS; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]);

    if (reading != buttonState[i]) {
      if (now - buttonDebounceTime[i] >= DEBOUNCE_MS) {
        buttonState[i] = reading;
        buttonDebounceTime[i] = now;

        if (reading == LOW) {
          // --- Momentary: Note On ---
          usbMIDI.sendNoteOn(
            BUTTON_NOTE_BASE + i,
            VELOCITY_ON,
            MIDI_CHANNEL
          );

          // --- Toggle: flip state and send ---
          buttonToggleOn[i] = !buttonToggleOn[i];
          if (buttonToggleOn[i]) {
            usbMIDI.sendNoteOn(
              BUTTON_TOGGLE_BASE + i,
              VELOCITY_ON,
              MIDI_CHANNEL
            );
          } else {
            usbMIDI.sendNoteOff(
              BUTTON_TOGGLE_BASE + i,
              VELOCITY_OFF,
              MIDI_CHANNEL
            );
          }
        } else {
          // --- Momentary: Note Off ---
          usbMIDI.sendNoteOff(
            BUTTON_NOTE_BASE + i,
            VELOCITY_OFF,
            MIDI_CHANNEL
          );
          // Toggle note does NOT change on release — it's a toggle
        }
      }
    }
  }
}
