/*****************************************************************************/
//  HighLevelExample.ino
//  Hardware:      Grove - 6-Axis Accelerometer&Gyroscope
//	Arduino IDE:   Arduino-1.65
//	Author:	       Lambor
//	Date: 	       Oct,2015
//	Version:       v1.0
//
//  Modified by:
//  Data:
//  Description:
//
//	by www.seeedstudio.com
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
/*******************************************************************************/

#include "LSM6DS3.h"
#include "Wire.h"

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A

// Setup variables
const int numReadings = 1000;
const int buttonPin = 0;
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;

bool recording = false;

// Button state varialbles
bool lastButtonState = LOW;
bool buttonState = LOW;
unsigned long lastDebounceTime = 0;
float buttonPressStart;
float buttonPressDuration;
bool newButtonSequence = false;
const unsigned long debounceDelay = 50;
int consecutiveButtonPresses = 1;
unsigned long consecutivePressTimer = 0;
const unsigned long consecutivePressThreshold = 250;

// Sensor mode variables
bool inBluetoothPairingMode = false;
bool inDataRecordMode = false;

// Indicator Light Timers
unsigned long ledRedTimer = 0;
unsigned long ledBlueTimer = 0;
unsigned long ledGreenTimer = 0;

void checkForButtonInputs() {
    // This section reads information about the interface button.
    // Results of this logic dictate what functions to do later in the code

    bool reading = digitalRead(buttonPin);
    // Serial.println(currentButtonState);

    // Check if button state has changed
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    // Check if enough time has passed since last state change
    if ((millis() - lastDebounceTime) > debounceDelay) {
      // If button state has changed we enter this area 
      if (reading != buttonState) {
        buttonState = reading;

        // Recognize toggle event if button is High
        if (buttonState == HIGH) {
          newButtonSequence = true;
          if (consecutivePressTimer != 0 && (millis() - consecutivePressTimer) < consecutivePressThreshold) {
            consecutiveButtonPresses++;

          }
          
          buttonPressStart = millis();
        }

        // If state changed and results in LOW, button has been released
        if (buttonState == LOW) {
          buttonPressDuration = (millis() - buttonPressStart) / 1000;
          consecutivePressTimer = millis();
          // Serial.print("Button Pressed for ");
          // Serial.print(buttonPressDuration);
          // Serial.println("s");

        }
      } 
    }

    lastButtonState = reading;
}

void ledBlink(int timeOn, int timeOff, pin_size_t ledPin, long unsigned &timerVariable) {
  // Serial.print(ledPin);
  if ((millis() - timerVariable) < timeOn) {
    digitalWrite(ledPin, LOW);
  } 

  else if ((millis() - timerVariable) >= timeOn && (millis() - timerVariable) <= timeOff) {
    digitalWrite(ledPin, HIGH);
  }

  else {
    timerVariable = millis();
  }
}

void testLED(int ledPin) {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}


void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    
    while (!Serial);
    //Call .begin() to configure the IMUs
    if (myIMU.begin() != 0) {
        Serial.println("Device error");
    } else {
        Serial.println("Device OK!");
    }
    
    pinMode(buttonPin, INPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);


}

void loop() {
    checkForButtonInputs();

    // If we're here then we're in on/standby mode. Green flashing 100ms High 900ms Low
    // testLED(13);
    // digitalWrite(14, LOW);
    /*
    if ((millis() - greenLightTimer) < 100) {
      digitalWrite(LED_GREEN, LOW);
    } 

    else if ((millis() - greenLightTimer) >= 100 && (millis() - greenLightTimer) <= 1900) {
      digitalWrite(LED_GREEN, HIGH);
    }

    else {
      greenLightTimer = millis();
    }
    */

    // Only Enter the next area once the consecutive press threshold has passed.
    // This way we only act on the info from the end of the sequence
    if ((newButtonSequence == true) && ((millis() - consecutivePressTimer) > consecutivePressThreshold) && (buttonState == LOW)) {
      Serial.print("Button Pressed ");
      Serial.print(consecutiveButtonPresses);
      Serial.println(" times.");
      Serial.print("Last press held for ");
      Serial.print(buttonPressDuration);
      Serial.println("s");

      if (consecutiveButtonPresses == 2) {
        digitalWrite(GREEN_LED, HIGH);
        if (buttonPressDuration > 3) {
          Serial.println("Entering Bluetooth Pairing Mode!");
          inBluetoothPairingMode = true;
        }

        else if (buttonPressDuration < 3) {
          Serial.println("Entering Data Record Mode!");
          inDataRecordMode = true;
        }
      }

      if (consecutiveButtonPresses == 1 && buttonPressDuration > 3) {
        if (inDataRecordMode == true) {
          Serial.println("Exiting Data Recording Mode!");
          inDataRecordMode = false;
          digitalWrite(RED_LED, HIGH);
        }

        else if (inBluetoothPairingMode == true) {
          Serial.println("Exiting Bluetooth Pairing Mode!");
          inBluetoothPairingMode = false;
          digitalWrite(BLUE_LED, HIGH);
        }
      }
        
      
      newButtonSequence = false;
      consecutiveButtonPresses = 1;
    }

  if (inBluetoothPairingMode == true) {
    ledBlink(100, 1900, BLUE_LED, ledBlueTimer);
    // Other stuff to do in bluetooth pairing mode
  }

  else if (inDataRecordMode == true) {
    ledBlink(100, 1900, RED_LED, ledRedTimer);
    // Other stuff to do in data recording mode
  }

  else {
    // Nothing Happening, in standby mode
    ledBlink(100, 1900, GREEN_LED, ledGreenTimer);
  }

    /*
    startTime = millis();
    for (int i = 0; i < numReadings; i++) {
      accelX = myIMU.readFloatAccelX();
    }
    endTime = millis();

    unsigned long elapsedTime = endTime - startTime;
    avgSampleRate = (numReadings / (elapsedTime / 1000.0));
    //Accelerometer
    Serial.print("Time Taken:");
    Serial.print(elapsedTime);
    Serial.println(" milliseconds");

    Serial.print("Average sampling rate: ");
    Serial.print(avgSampleRate);
    Serial.println(" readings per second");

    delay(1000);

    */
}
