// Test 1: Serial Hello
// Purpose: Verify Teensy is alive, USB works, serial monitor connected.
// Upload this FIRST after soldering. If you see "Dronemaker alive!" in
// serial monitor, the Teensy and USB connection are working.
//
// Arduino IDE settings:
//   Board: Teensy 4.1
//   USB Type: Serial (for tests 1-5) or Serial + MIDI (for test 6)

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  Serial.println("Dronemaker alive!");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
