/*****************************************************************************/
/*  
  XIAO_Feature_Integration.ino
  Hardware:      Seeed XIAO BLE Sense - nRF52840
                 SD Card SPI Breakout Board
	Arduino IDE:   Arduino-2.3.4
	Author:	       pcavanaugh04
	Date: 	       Dec,2024
	Version:       v1.0

  This tutorial is the beginning of inegrating features from previous  
  tutorials of my swIMU project. This series is aimed to teach some of the
  fundamentals of product design in sports technology applications, including
  user interfaces, technical interfaces, and technical implementations. 

  If you are just getting started... Go back to the first example!
  
  This module integrates the device config, data recording, user interface, and 
  BLE transmissions thats been introduced in the last 7 modules. This demonstrates
  the additional considerations that come into play when integrateing 
  individual functions at the system level, and ensureing the orignal
  functionality is preserved within the component interactions.

  We've defined a few primary functions of the device so far:
  - Measure Accelerometer Data
  - Record Accelerometer Data
  - Transmit Accelerometer Data
  
  Up until this point, we've integrated functionality of measuring and recording
  acclelerometer data, and demonstrated transmitting data. This
  integration will focus on integrating measuring <-> transmitting and 
  recording <-> transmitting, while enabling full control of the
  device via the button interface. 

  At a high level, we will have 5 "modes":
  - On/Standby
  - Accel Data Measure + Tx + Record
  - Accel Data Measure + Record
  - BLE Config
  - BLE File Tx

  A breakdown of each mode will be written at each definition
  
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

#include <string.h>
#include <ArduinoBLE.h>
#include "DataRecorder.h"
#include "BLEManager.h"
#include "LEDManager.h"

// Setup variables
const int buttonPin = 0;  // I/O pin where the button is connected
// Pin assignments of the onbaord LED
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;
const int chipSelect = 5;  // Digital I/O pin needed for the SPI breakout

// Create instance of a DataRecorder class to encapsulate data reading and file recording
DataRecorder dataRecorder = DataRecorder();
BLEManager bleManager = BLEManager(dataRecorder);
LEDManager ledManager = LEDManager(RED_LED, GREEN_LED, BLUE_LED);
// const unsigned long microsOverflowValue = 4294967295;

// Button state varialbles - Functions reviewed in previous tutorials
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
bool inDataRecordTxMode = false;
int dataRecordTxSequence[] = {400, 100, 400, 100};
bool inDataRecordMode = false;
int dataRecordSequence[] = {1900, 100};
bool inBLEConfigMode = false;
int bleConfigSequence[] = {100, 1400, 100, 100};
bool inFileTxMode = false;
int fileTxSequence[] = {400, 100};
bool inStandbyMode = true;
int standbySequence[] = {1900, 100};
int connectionTimeout = 20; // Timeout [s] for BLE connection search


void returnToStandbyMode() {
  // Function to be called when modes change, ensures clean exit of respective modes
  ledManager.turnOff();

  if (inFileTxMode == true) {
    inFileTxMode = false;
    bleManager.exitFileTxMode();
  // Code to clean disconnect from client
  }

  else if (inDataRecordTxMode == true) {
    inDataRecordTxMode = false;
    Serial.println("Exiting IMU Tx and Record Mode!");
    bleManager.exitIMUTxRecordMode();
  // Code to clean disconnect from client
  // Code to save and close file
  // Update whitelist file with new filenames
  }

  else if (inBLEConfigMode == true) {
    Serial.println("Exiting Bluetooth Config Mode!");
    inBLEConfigMode = false;
    bleManager.exitConfigMode();
    // Code to clean disconnect from client
  }

  else if (inDataRecordMode == true) {
    inDataRecordMode = false;
    Serial.println("Exiting Data Recording Mode!");
    dataRecorder.stopDataRecording(bleManager.dataFileName.c_str());
  // Code to clean disconnect from client
  // Code to save and close file
  // Update whitelist file with new filenames
  }
  
  inStandbyMode = true;

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
  // This function checks conditions of button events and sets the necessary flags
  // to indicate the resulting change of state to the device or operating mode

  // Only handle and event if a button sequence has been completed
  if ((newButtonSequence == true) && ((millis() - consecutivePressTimer) > consecutivePressThreshold) && (buttonState == LOW)) {
    Serial.print("Button Pressed ");
    Serial.print(consecutiveButtonPresses);
    Serial.println(" times.");
    Serial.print("Last press held for ");
    Serial.print(buttonPressDuration);
    Serial.println("s");
    ledManager.turnOff();

  // 2 consecutive button presses indicate the start of a new operating mode.
  // Only accept a change in operating mode if coming from standby mode
    if ((consecutiveButtonPresses == 2) && (inStandbyMode)) {
      inStandbyMode = false;

  // A long hold indicates going into bluetooth pairing/transmit mode  
      if (buttonPressDuration > 3) {
        bleManager.enterIMUTxRecordMode(connectionTimeout);
        Serial.println("Entering Live IMU BLE Tx Mode!");
        inDataRecordTxMode = true;

      }
  // A short hold indicates starting accelerometer data recording
      else {
        Serial.println("Entering Data Record Mode!");
        inDataRecordMode = true;
        // Have some check to see if a valid file name has been recieved
        String dataFileName = bleManager.updateFileName();
        dataRecorder.startDataRecording(dataFileName.c_str());
        }
      }
    else if ((consecutiveButtonPresses == 3) && (inStandbyMode)) {
      inStandbyMode = false;
      // 3 button presses enters pure BLE funcitonality. <3s on last press is config mode, >3s is fileTXMode
      if (buttonPressDuration < 3) {
        // Serial.println("Entering Bluetooth Pairing Mode!");
        inBLEConfigMode = true;
        bleManager.enterConfigMode(connectionTimeout);
        Serial.println("Entering BLE Config Mode.");

        //bleManager.enterPairingMode(10);
          // Perform connection procedure
          // Serial.println("Entering Bluetooth Config Mode.
      }

      else {
        inFileTxMode = true;
        bleManager.enterFileTxMode(connectionTimeout);
        //Perform connection procedure
        Serial.println("Entering BLE File Transfer Mode.");
      }
    }

    // Single press and hold indicates an exit event. Code will exit from whichever mode was previously active
    else if (consecutiveButtonPresses == 1 && buttonPressDuration > 3) {
      if (buttonPressDuration > 3) {
        Serial.println("Returning to Standby Mode");
        returnToStandbyMode();
      }
      /*
      else if ((buttonPressDuration >=10) && inStandbyMode) {
        // We can only enter sleep mode if coming from standby mode. This ensures clean exits of any of
        // the mode functions. Sleep mode is configured to disable all of the periphrials and delay the loop
        // for a bit every loop cycle. A wakeup event will reset the system.

        Potential implementation if desired to implement a sleep mode
      }
      */
      
    } 
       
    // Reset variables at end of handling sequence
    newButtonSequence = false;
    consecutiveButtonPresses = 1;   
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // ledManager.initialize();
  // while(!Serial) // This code ensures the program only runs if the serial monitor is open. Uncomment if desired
    // Initialize Input Buttons
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);

  delay(1000);
  dataRecorder.initDevices(chipSelect);
  bleManager.startBLE();
  delay(1000);
  dataRecorder.displayDirectory("accelDir");
}

void loop() {
  // Start each loop by checking state of button inputs
  checkForButtonInputs();
  // Check if any events need to be handled
  handleButtonEvent();

  // Depending on mode, do different things
  if (inBLEConfigMode) {
    ledManager.blink(bleConfigSequence, sizeof(bleConfigSequence), "B");
    if (bleManager.inPairingMode) {
      bleManager.pairCentral();
      if (bleManager.reachedTimeout) {
        returnToStandbyMode();
      }
    }

    else {
      bleManager.poll();
      if (bleManager.fileConfigedFlag) {
        bleManager.exitConfigMode();
        returnToStandbyMode();
      }
    }
  }

  else if (inDataRecordMode) {
    ledManager.blink(dataRecordSequence, sizeof(dataRecordSequence), "R");
    char* dataLine = dataRecorder.readIMU();
    // Serial.println(dataLine);
    }

  else if (inDataRecordTxMode) {
    ledManager.blink(dataRecordTxSequence, sizeof(dataRecordTxSequence), "RB");
    if (bleManager.inPairingMode) {
      bleManager.pairCentral();
      if (bleManager.reachedTimeout) {
        returnToStandbyMode();
      }
    }
    else {
      if (!bleManager.imuRecordandTx()) {
        returnToStandbyMode();
      }
    }
  }

  else if (inFileTxMode) {
    ledManager.blink(fileTxSequence, sizeof(fileTxSequence), "B");
    bleManager.poll();

    if (bleManager.inPairingMode) {
      bleManager.pairCentral();
      if (bleManager.reachedTimeout) {
        returnToStandbyMode();
      }
    }  
    else if (!bleManager.txFileData()) {
      returnToStandbyMode();
    }
  }

  else if (inStandbyMode) {
    // Nothing Happening, in standby mode
    ledManager.blink(standbySequence, sizeof(standbySequence), "G");
  }
}
