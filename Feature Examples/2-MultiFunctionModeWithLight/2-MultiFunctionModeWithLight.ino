/*****************************************************************************/
/*  
  2-MultiFunctionModeWithLight.ino
  Hardware:      Seeed XIAO BLE Sense - nRF52840
	Arduino IDE:   Arduino-2.3.4
	Author:	       pcavanaugh04
	Date: 	       Dec,2024
	Version:       v1.0

  This tutorial is the second in the series of understanding some of the 
  functions of my swIMU project. This series is aimed to teach some of the
  fundamentals of product design in sports technology applications, including
  user interfaces, technical interfaces, and technical implementations. 

  If you are just getting started... Go back to the first example!
  
  This module extends on the toggle button feature to handle a few different
  input configurations and update the state of the device accordingly. This adds
  onboard RGB light as an output component of the UI, which indicates to the user
  information about the state of the device. With the RGB LED, we have 2 dimensions
  of configuration - the color of the device and the flashing sequence. Similar to
  how we can create a large number of combinations of inputs with number of button
  presses and duration of presses on the input, we can create a large number of
  indications with the color and flash sequence of the onboard LED.
  
  The onbaord LED is a common cathode. This means that we can turn the R, G, or B
  lights on by setting their corresponding pins to "LOW" to complete the circuit
  through the LED.

  This module introduces a new function, ledBlink, to configure blink sequences
  to indicate states
  
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
// Pin assignments of the onbaord LED
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;

// Button state varialbles
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

// Sensor mode flags to indicate what state the device is in
bool inBluetoothPairingMode = false;
bool inDataRecordMode = false;

// Indicator Light Variables
int defaultSequence[] = {1900, 100, 500, 500};
unsigned long ledTimer = 0;
int sequenceIndex = 0;
bool ledOn = false;

void ledBlink(int sequenceArray[], int &sequenceInd, pin_size_t ledPin, long unsigned &timerVariable, bool &ledOn) {
  // Function to make a specified LED blink at a specified duty cycle. sequenceArray is an array of integer values
  // Structured as [timeOff, timeOn, timeOff, timeOn....] for the desired blink pattern.

  int segmentDuration = sequenceArray[sequenceInd];
  unsigned int currentDuration = millis() - timerVariable;

  // If LED is off and the timier < offTime, turn it on, track as an event, advance the counter
  else if (!ledOn && (segmentDuration <= currentDuration)) {
    digitalWrite(ledPin, LOW);
    ledOn = true;
    timerVariable = millis();
    Serial.println("Turning LED ON");
    if (sequenceInd < (sizeof(sequenceArray) - 1)) {
      sequenceInd++;
    }

    else {
      sequenceInd = 0;
    }
  }

  // If LED is on and the timer > onTime, turn it off, track as an event, advance the counter
  if (ledOn && (segmentDuration <= currentDuration)) {
    Serial.println("Turning LED OFF");
    digitalWrite(ledPin, HIGH);
    ledOn = false;
    timerVariable = millis();
    if (sequenceInd < (sizeof(sequenceArray) - 1)) {
      sequenceInd++;
    }

    else {
      sequenceInd = 0;
    }
  }

  else {
    Serial.println("Uncaught Case! Whats going on???");
  }
}

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
    
  // Based on the results of the button event, update device states accordingly
  // This is also where any one-time code to configure a mode will run
  
  // 2 consecutive button presses indicate the start of a new operating mode
    if (consecutiveButtonPresses == 2) {
      // Turn off all LEDs
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);

      // Reset LED counter/tracker variables
      ledOn = false;
      sequenceIndex = 0;
      ledTimer = millis();

  // A long hold indicates going into bluetooth transmit mode  
      if (buttonPressDuration > 3) {
        Serial.println("Entering Bluetooth Pairing Mode!");
        inBluetoothPairingMode = true;
      }
  // A short hold indicates starting accelerometer data recording
      else if (buttonPressDuration < 3) {
        Serial.println("Entering Data Record Mode!");
        inDataRecordMode = true;
        }
    }
    
    // Single press and hold indicates an exit event. Code will exit from whichever mode was previously active
    if (consecutiveButtonPresses == 1 && buttonPressDuration > 3) {
      // Turn off all LEDs
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);

      // Reset LED counter/tracker variables
      sequenceIndex = 0;
      ledOn = false;
      ledTimer = millis();
      
      if (inDataRecordMode == true) {
        Serial.println("Exiting Data Recording Mode!");
        inDataRecordMode = false;
        digitalWrite(RED_LED, HIGH);  // Turnoff Red LED
      }

      else if (inBluetoothPairingMode == true) {
        Serial.println("Exiting Bluetooth Pairing Mode!");
        inBluetoothPairingMode = false;
        digitalWrite(BLUE_LED, HIGH);  // Turn off Blue LED
      }
    }
        
    // Reset variables at end of handling sequence
    newButtonSequence = false;
    consecutiveButtonPresses = 1;
  }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);  // Setup serial for debugging
    
    pinMode(buttonPin, INPUT);  // Configure button input pin
    // Config onboard led
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    // Make sure LED starts off
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);

}

void loop() {
  // This code runs in a loop. Each time the loop runs our state variables 
  // are checked and timers increment appropriately
  checkForButtonInputs();
  handleButtonEvent();

  // Based on the states of our flag variables, perform appropriate functions
  if (inBluetoothPairingMode == true) {
    ledBlink(defaultSequence, sequenceIndex, BLUE_LED, ledTimer, ledOn);
    // Other stuff to do in bluetooth pairing mode
  }

  else if (inDataRecordMode == true) {
    ledBlink(defaultSequence, sequenceIndex, RED_LED, ledTimer, ledOn);
    // Other stuff to do in data recording mode
  }

  else {
    // Nothing Happening, in standby mode
    ledBlink(defaultSequence, sequenceIndex, GREEN_LED, ledTimer, ledOn);
  }
}
