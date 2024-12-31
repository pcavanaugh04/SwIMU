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
// #include "BLEManager.h"

// Setup variables
const int buttonPin = 0;  // I/O pin where the button is connected
// Pin assignments of the onbaord LED
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;
const int chipSelect = 5;  // Digital I/O pin needed for the SPI breakout

// Create instance of a DataRecorder class to encapsulate data reading and file recording
DataRecorder dataRecorder = DataRecorder();
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
bool inDataRecordMode = false;
bool inBLEConfigMode = false;
bool inBLETxMode = false;

// Indicator Light Timers
unsigned long ledRedTimer = 0;
unsigned long ledBlueTimer = 0;
unsigned long ledGreenTimer = 0;

// ----------------------------- BLE Configs ---------------------------- //
// File Config Service and Characteristics
const char* configInfoServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
const char* dateTimeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440001";
const char* personNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440002";
const char* activityTypeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440003";
const char* fileNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440004";
BLEStringCharacteristic dateTimeConfigChar(dateTimeConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic personNameConfigChar(personNameConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic activityTypeConfigChar(activityTypeConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic fileNameConfigChar(fileNameConfigCharUuid, BLERead, 80);

// IMU Data Stream Service and Characteristics
const char* IMUServiceUuid = "550e8402-e29b-41d4-a716-446655440000";
const char* IMURequestCharacteristicUuid = "550e8403-e29b-41d4-a716-446655440001";
const char* IMUDataCharacteristicUuid = "550e8403-e29b-41d4-a716-446655440002";
BLEService imuService(IMUServiceUuid);
BLEStringCharacteristic imuRequestCharacteristic(IMURequestCharacteristicUuid, BLEWrite, 10);
BLEStringCharacteristic imuDataCharacteristic(IMUDataCharacteristicUuid, BLERead | BLENotify, 100);

// File Transfer Service and Characteristics
const char* fileTransferServiceUuid = "550e8404-e29b-41d4-a716-446655440000";
const char* fileTransferRequestCharUuid = "550e8405-e29b-41d4-a716-446655440001";
const char* fileTransferDataCharUuid = "550e8405-e29b-41d4-a716-446655440002";
const char* fileTransferCompleteCharUuid = "550e8405-e29b-41d4-a716-446655440003";
const char* fileNameResponseCharUuid = "550e8405-e29b-41d4-a716-446655440004";
const int fileTxBufferSize = 244; // Oddly enough this seems to be the bandwidth of the string Char
// Init Objects
BLEService fileTransferService(fileTransferServiceUuid);
BLEStringCharacteristic fileTransferRequestChar(fileTransferRequestCharUuid, BLEWrite, 10);
BLECharacteristic fileTransferDataChar(fileTransferDataCharUuid, BLENotify, fileTxBufferSize, false);
BLEStringCharacteristic fileTransferCompleteChar(fileTransferCompleteCharUuid, BLENotify, 30);
BLEStringCharacteristic fileNameResponseChar(fileNameResponseCharUuid, BLERead, 60);


//// going to need a good way to organize all this so it doesnt get spaghetti
// Thinking of splitting things into modules, but dont see a huge need for custom classes. Yet
// DataRecording.h
//    IMU setup and recording stuff, also extends into calibration when we get there
//    SD card functions and such

// BLEModules.h
//    Callbacks
//    handling data (bytes2str)
//    file config info and storage of variables

// We'll keep this script as the highest level integrations
void ledBlink(int timeOn, int timeOff, pin_size_t ledPin, long unsigned &timerVariable) {
  // Function to make a specified LED blink at a specified duty cycle 
 
  // If the current timerVariable has been on less than the specified on/time, keep the light on
  if ((millis() - timerVariable) < timeOn) {
    digitalWrite(ledPin, LOW);
  } 

  // Otherwise turn/keep it off
  else if ((millis() - timerVariable) >= timeOn && (millis() - timerVariable) <= timeOff) {
    digitalWrite(ledPin, HIGH);
  }

  else {
    timerVariable = millis();
  }
}

void modeExitChecks(){
  // Function to be called when modes change, ensures clean exit of respective modes
  if (inBLETxMode == true) {
  inBLETxMode = false;
  // Code to clean disconnect from client
  }

  else if (inDataRecordTxMode == true) {
  inDataRecordTxMode = false;
  // Code to clean disconnect from client
  // Code to save and close file
  // Update whitelist file with new filenames
  }

  else if (inBLEConfigMode == true) {
    Serial.println("Exiting Bluetooth Pairing Mode!");
    inBLEConfigMode = false;
    digitalWrite(BLUE_LED, HIGH);  // Code to clean disconnect from client
  }

  else if (inDataRecordMode == true) {
    inDataRecordMode = false;
    Serial.println("Exiting Data Recording Mode!");
    digitalWrite(RED_LED, HIGH);
    dataRecorder.stopDataRecording();
  // Code to clean disconnect from client
  // Code to save and close file
  // Update whitelist file with new filenames
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
  // 2 consecutive button presses indicate the start of a new operating mode
    if (consecutiveButtonPresses == 2) {
      digitalWrite(GREEN_LED, HIGH);

  // A long hold indicates going into bluetooth pairing/transmit mode  
      if (buttonPressDuration > 3) {
        Serial.println("Entering Bluetooth Pairing Mode!");
        inBLEConfigMode = true;
      }
  // A short hold indicates starting accelerometer data recording
      else if (buttonPressDuration < 3) {
        Serial.println("Entering Data Record Mode!");
        inDataRecordMode = true;
        const char* defaultName = "defaultName.csv";
        dataRecorder.startDataRecording(defaultName);
        }
      }
    // Single press and hold indicates an exit event. Code will exit from whichever mode was previously active
    if (consecutiveButtonPresses == 1 && buttonPressDuration > 3) {
      modeExitChecks();
    } 
       
    // Reset variables at end of handling sequence
    newButtonSequence = false;
    consecutiveButtonPresses = 1;   
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // while(!Serial) // This code ensures the program only runs if the serial monitor is open. Uncomment if desired
    // Initialize Input Buttons
  pinMode(buttonPin, INPUT);
  pinMode(chipSelect, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, HIGH);
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(chipSelect, HIGH);

  delay(1000);
  dataRecorder.initDevices(chipSelect);
  dataRecorder.displayDirectory();
}

void loop() {
  // Start each loop by checking state of button inputs
  checkForButtonInputs();
  // Check if any events need to be handled
  handleButtonEvent();

  // Depending on mode, do different things
  if (inBLEConfigMode == true) {
    ledBlink(100, 1900, BLUE_LED, ledBlueTimer);
    // Other stuff to do in bluetooth pairing mode... COMING SOON
  }

  else if (inDataRecordMode == true) {
    ledBlink(100, 1900, RED_LED, ledRedTimer);
    // Check to see if assigned data rate has been met
    char* dataLine = dataRecorder.readIMU();
    Serial.println(dataLine);
    }

  else {
    // Nothing Happening, in standby mode
    ledBlink(100, 1900, GREEN_LED, ledGreenTimer);
  }
}
