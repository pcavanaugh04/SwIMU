/*****************************************************************************/
/*  1-ToggleButtonTest.ino
  Hardware:      Seeed XIAO BLE Sense - nRF52840
	Arduino IDE:   Arduino-2.3.4
	Author:	       pcavanaugh04
	Date: 	       Dec,2024
	Version:       v1.0

  This tutorial is the first in the series of understanding some of the 
  functions of my swIMU project. This series is aimed to teach some of the
  fundamentals of product design in sports technology applications, including
  user interfaces, technical interfaces, and technical implementations. 

  If you are just getting started, ensure that you have properly configured
  Arduino IDE to support the Seeed Studio hardware. Instructions and
  quickstart guide can be found here: https://wiki.seeedstudio.com/XIAO_BLE/
  
  This module demonstrates a simple toggle button feature using a pushbutton.
  A push button is a simple hardware user interface (UI) component. Pressing the
  button completes a circuit between a digital I/O pin and VCC or GND, indicating
  an event has occurred. A toggle button uses additonal logic to detect the
  current state of a variable and decide how the button press should update the
  state of the variable. This code demonstrates how we setup and read a 
  sequence of button presses from the user. In the next tutorial, we will config
  the device to enter different operating modes based on the user input. 
  
  This hardware is configured with LOW as off and HIGH as on. There is a 10k 
  pulldown resistor on the I/O pin to ensure a clean LOW on the pin when the
  button is not engaged.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*******************************************************************************/

// Setup variables
const int buttonPin = 0; // I/O pin where the button is connected

// Button state varialble declarations
bool lastButtonState = LOW;  // State variable of last button press
bool buttonState = LOW;  // Current button state
unsigned long lastDebounceTime = 0;  // timestamp of last button event (either press or release)
float buttonPressStart;  // time index to record the start of button event
float buttonPressDuration;  // Store how long the button has been pressed
bool newButtonSequence = false;  // Flag to indicate if a new sequence has been detected for evaluation
const unsigned long debounceDelay = 50;  // Time limit to filter out any signal errors from the button press
int consecutiveButtonPresses = 1;  // Counter variable to detect consecutive presses
unsigned long consecutivePressTimer = 0;  // Store how long a button is held down
const unsigned long consecutivePressThreshold = 250;  // Threshold for determining when to count button presses as consecutive

// Function to detect sequence and duration of button presses
void checkForButtonInputs() {
    // Reads information about user interaction with the input button.
    // This input is setup to accept a number of consecutive button presses and
    // count the duration of the last button press. This gives 2 dimensions of 
    // inputs: number of presses, and duration of last button press. Different 
    // combinations of these two events can be assigned to functions, which will
    // demonstrated later in the tutorial

    // Read current state of the button pin
    bool reading = digitalRead(buttonPin);
    // Serial.println(currentButtonState); // Optional debug statement

    // Check if button state has changed
    if (reading != lastButtonState) {
      // If state has changed, start a counter to filter any change values within the debounce delay
      lastDebounceTime = millis();
    }

    // Check if enough time has passed since last state change
    if ((millis() - lastDebounceTime) > debounceDelay) {
      // If a successful button state changed is detected we enter this area 
      if (reading != buttonState) {
        buttonState = reading; // update buttonState to the current reading

        // Recognize the start of a toggle event if button is High
        if (buttonState == HIGH) {
          newButtonSequence = true;
          // If consecutive presses happen within the threshold, increment the press counter
          if (consecutivePressTimer != 0 && (millis() - consecutivePressTimer) < consecutivePressThreshold) {
            consecutiveButtonPresses++;

          }
          // start a timer to count how long the final press duration is
          buttonPressStart = millis();
        }

        // If state changed and results in LOW, button has been released
        if (buttonState == LOW) {
          buttonPressDuration = (millis() - buttonPressStart) / 1000;
          consecutivePressTimer = millis();
        }
      } 
    }
    // update the last button state to the current reading, for when everything is checked in the next loop iteration
    lastButtonState = reading;
}

void handleButtonEvent() {
  // Function to recongize if a new and valid button sequence is available
  if ((newButtonSequence == true) && ((millis() - consecutivePressTimer) > consecutivePressThreshold) && (buttonState == LOW)) {
    Serial.print("Button Pressed ");
    Serial.print(consecutiveButtonPresses);
    Serial.println(" times.");
    Serial.print("Last press held for ");
    Serial.print(buttonPressDuration);
    Serial.println("s");
    
    // Here is where code will go to handle different events

    // Reset flags after event has been handled
    newButtonSequence = false;
    consecutiveButtonPresses = 1;
    }
  }

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);  // Setup serial for debugging
    
    pinMode(buttonPin, INPUT);  // Configure button input pin
}

void loop() {
    // This code runs in a loop. Each time the loop runs our state variables 
    // are checked and timers increment appropriately
    checkForButtonInputs();
    handleButtonEvent();
}
