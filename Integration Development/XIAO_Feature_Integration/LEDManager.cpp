#include "LEDManager.h"

LEDManager::LEDManager(pin_size_t RED_LED, pin_size_t GREEN_LED, pin_size_t BLUE_LED):
  redLED(RED_LED), greenLED(GREEN_LED), blueLED(BLUE_LED) {

}

void LEDManager::blink(int sequenceArray[], char LEDColors[]) {
  // blink the onboard LED in a predefined sequence structured as [timeOff, timeOn, timeOff, timeOn....]
  // Use a char {"R", "G", "B" to determine which LED to address}, multiple colors can be specified if a char array is used, and they will
  // Align to the sequence array as off/on sequence to first letter, 2nd sequence to second, etc

  LED* blinkLED = nullptr;
  int sequenceCount = sequenceIndex / 2;
  char colorIndex;
  
  // char FixedLEDColors[] = "G";

  if (strlen(LEDColors) == 1) {
    colorIndex = LEDColors[0];
  }

  else {
    colorIndex = LEDColors[sequenceCount];
  }
  // Serial.println(sequenceIndex);
  // Serial.println(LEDColors);
  // Serial.println((int)colorIndex);

  switch (colorIndex) {
    case 'R':
      blinkLED = &redLED;
      break;

    case 'G':
      blinkLED = &greenLED;
      break;

    case 'B':
      blinkLED = &blueLED;
      break;

    default:
      Serial.println("Invalid LED Selection!");
      return;  
  }
  if (blinkLED) {
    // Function to make a specified LED blink at a specified duty cycle. sequenceArray is an array of integer values
    // Structured as [timeOff, timeOn, timeOff, timeOn....] for the desired blink pattern.

    int segmentDuration = sequenceArray[sequenceIndex];
    unsigned int currentDuration = millis() - LEDTimer;

    // If LED is off and the timier < offTime, turn it on, track as an event, advance the counter
    if ((!blinkLED->isOn) && (segmentDuration <= currentDuration)) {
      blinkLED->turnOn();
      LEDTimer = millis();
      // Serial.println("Turning LED ON");

      if (sequenceIndex < (sizeof(sequenceArray) / sizeof(sequenceArray[0]))) {
        sequenceIndex++;
      }

      else {
        sequenceIndex = 0;
      }
    }

    // If LED is on and the timer > onTime, turn it off, track as an event, advance the counter
    else if ((blinkLED->isOn) && (segmentDuration <= currentDuration)) {
      // Serial.println("Turning LED OFF");
      blinkLED->turnOff();
      LEDTimer = millis();
    
      if (sequenceIndex < (sizeof(sequenceArray) / sizeof(sequenceArray[0]))) {
        sequenceIndex++;
      }

      else {
        sequenceIndex = 0;
      }
    }
  }
}

void LEDManager::turnOff() {
  redLED.turnOff();
  greenLED.turnOff();
  blueLED.turnOff();
  sequenceIndex = 0;
  LEDTimer = millis();

}


LED::LED(pin_size_t ledPin) {
  pin = ledPin;
  pinMode(pin, OUTPUT);
  turnOff();
}

void LED::turnOff() {
  digitalWrite(pin, HIGH);
  isOn = false;
}

void LED::turnOn() {
  digitalWrite(pin, LOW);
  isOn = true;
}