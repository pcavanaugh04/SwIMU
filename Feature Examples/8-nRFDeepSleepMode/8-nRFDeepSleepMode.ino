#include <Arduino.h>

const int buttonPin = 0;        // Button input pin
const int ledPin = LED_BUILTIN; // Built-in LED for feedback
const unsigned long holdTime = 6000; // 6 seconds hold time
volatile bool sleepFlag = false;
volatile bool buttonPressDetected = false;
volatile bool longPressDetected = false;
volatile bool fallingEdgeInterruptDetected = false;
volatile bool ledOn = false;
unsigned long ledMillis = 0;
unsigned long pressStartTime = 0;


void setup() {
  // Serial.begin(115200);
  // while (!Serial);
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, CHANGE);
  Serial.println("Setup and ready to go!");
  ledMillis = millis();
}

void loop() {

  if (buttonPressDetected) {
    Serial.println("Button Press Recognized. Tracking Input");
    buttonPressDetected = false;

  }

  if (longPressDetected) {
    Serial.println("Long Press Detected");
    longPressDetected = false;
  }

  if (fallingEdgeInterruptDetected) {
    Serial.println("Falling Edge interrupt Detected, but no action!");
    fallingEdgeInterruptDetected = false;
  }

  if (sleepFlag) {
    Serial.println("Entering sleep mode!");
    enterSleepMode();
  }

  blinkWithoutDelay(1000);
  // Serial.println("End of loop!");
}

void blinkWithoutDelay(int blinkTimeMillis) {
  // If tracked time is greater than blink time, change states
  if ((millis() - ledMillis) > blinkTimeMillis) {
    ledOn = !ledOn;
    if (ledOn) {
      digitalWrite(ledPin, LOW);
    }
    else {
      digitalWrite(ledPin, HIGH);
    }
    ledMillis = millis();
  }
}


void buttonISR() {
  // Disable interrupts while updating flags
  noInterrupts();
  if (digitalRead(buttonPin) == HIGH) {
    buttonPressDetected = true;
  } 
  interrupts();
}

void enterSleepMode() {
  Serial.println("Entering Sleep Mode!");
  digitalWrite(ledPin, LOW);
  delay(100); // Small debounce delay

  while (sleepFlag) {
    __WFI();
    __SEV();
    __WFI();
  }

  digitalWrite(ledPin, HIGH);
}
