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

const int numReadings = 1000;
const int buttonPin = 0;
bool recording = false;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

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

    pinmode(buttonPin, INPUT);
}

void loop() {
    
    bool currentButtonState = digitalRead(buttonPin);

    // Check if button state has changed
    if (currentButtonState != lastButtonState) {
      lastDebounceTime = millis();
    }

    // Check if enough time has passed since last state change
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (currentButtonState == LOW && lastButtonState == HIGH) {
        recording = !recording // Toggle recording state
        Serial.print("Recording ");
        Serial.println(recording ? "started" : "stopped");
        delay(200);
      }
    }
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
}
