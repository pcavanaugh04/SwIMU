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

// #include "Arduino.h"
#include "LSM6DS3.h"
#include "Wire.h"
#include "SD.h"

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A

// Setup variables
const int numReadings = 1000;
const int buttonPin = 0;
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;
const int chipSelect = 2;
const unsigned long microsOverflowValue = 4294967295;

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

// Accelerometer Variables
unsigned long accelStartMillis;
unsigned long numSamples = 0;
float accelTime;
int currentCount;
File accelDataFile;
const float dataRateHz = 500.0;
const float dataPerioduS = 1 / dataRateHz * 1000000;
unsigned long dataReadTime;

void displayDirectory(File dir, int numTabs=0) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (int i=0; i < numTabs; i++)
      Serial.print('\t');

    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      displayDirectory(entry, numTabs + 1);
    }
    else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

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
        inBluetoothPairingMode = true;
      }
  // A short hold indicates starting accelerometer data recording
      else if (buttonPressDuration < 3) {
        Serial.println("Entering Data Record Mode!");
        inDataRecordMode = true;
        accelStartMillis = millis();
        // Section to Open New Data Record File
        // Check file counter count
        String accelDir = "accelDir/";
        File counterFile = SD.open(accelDir + "counter.txt", FILE_READ);
        Serial.println("Opened counter file!");
        if (counterFile) {
          currentCount = counterFile.parseInt();
          counterFile.close();
          currentCount++;
          char fileName[12];
          sprintf(fileName, "DATA_%d.csv", currentCount);
          Serial.print("Current file count number: ");
          Serial.println(currentCount);
          accelDataFile = SD.open(accelDir + fileName, FILE_WRITE);
          dataReadTime = micros();
          //accelDataFile.print("Test String!");
        }
      }
    }
  // Single press and hold indicates an exit event. Code will exit from whichever mode was previously active
      if (consecutiveButtonPresses == 1 && buttonPressDuration > 3) {
        if (inDataRecordMode == true) {
          Serial.println("Exiting Data Recording Mode!");
          inDataRecordMode = false;
          digitalWrite(RED_LED, HIGH);
          float calcHz = numSamples / accelTime;
          numSamples = 0;

          // Close Data File
          accelDataFile.close();

          // Save new counter variable to file
          SD.remove("accelDir/counter.txt");
          File newCounterFile = SD.open("accelDir/counter.txt", FILE_WRITE);
          newCounterFile.print(currentCount, DEC);
          newCounterFile.close();
          Serial.print("Calculated data rate [Hz]: ");
          Serial.println(calcHz);


        }

        else if (inBluetoothPairingMode == true) {
          Serial.println("Exiting Bluetooth Pairing Mode!");
          inBluetoothPairingMode = false;
          digitalWrite(BLUE_LED, HIGH);
        }
      }
        
  // Reset variables at end of handling sequence
    newButtonSequence = false;
    consecutiveButtonPresses = 1;
    
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial)
  digitalWrite(chipSelect, HIGH);
  delay(1000);
  Serial.println("Initializing SD Card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initializing failed!");
    while (1);
  }

  else {
    Serial.println("Initialization done.");
  }

  File root = SD.open("/");
  displayDirectory(root);
  root.close();

    //while (!Serial);
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
  // Start each loop by checking state of button inputs
  checkForButtonInputs();
  // Check if any events need to be handled
  handleButtonEvent();

  // Depending on mode, do different things
  if (inBluetoothPairingMode == true) {
    ledBlink(100, 1900, BLUE_LED, ledBlueTimer);
    // Other stuff to do in bluetooth pairing mode
  }

  else if (inDataRecordMode == true) {
    ledBlink(100, 1900, RED_LED, ledRedTimer);
    // Check to see if assigned data rate has been met
    unsigned int timeNowuS = micros();
    // Handle Overflow Event
    if (timeNowuS < dataReadTime) {
      timeNowuS += microsOverflowValue;
    }

    if ((timeNowuS - dataReadTime) > dataPerioduS * 6) {
      dataReadTime = micros();
      accelTime = (float)(millis() - accelStartMillis) / 1000;
      char accelBuffer[40];
      float accelX = myIMU.readFloatAccelX();
      float accelY = myIMU.readFloatAccelY();
      float accelZ = myIMU.readFloatAccelZ();
      float gyroX = myIMU.readFloatGyroX();
      float gyroY = myIMU.readFloatGyroY();
      float gyroZ = myIMU.readFloatGyroZ();
      sprintf(accelBuffer, "%.4f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", accelTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
      Serial.println(accelBuffer);
      accelDataFile.println(accelBuffer);
      numSamples = numSamples + 6;
    }
    /*
    // float accelDataLine[7] = {accelTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ};
    Serial.print("[");
    //Serial.print((float)(millis() - accelStartMillis) / 1000);
    Serial.print(accelTime);
    Serial.print(", ");
    Serial.print(accelX);
    Serial.print(", ");
    Serial.print(accelY);
    Serial.print(", ");
    Serial.print(accelZ);
    Serial.print(", ");
    Serial.print(gyroX);
    Serial.print(", ");
    Serial.print(gyroY);
    Serial.print(", ");
    Serial.print(gyroZ);
    Serial.print("]");
    */
    

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
