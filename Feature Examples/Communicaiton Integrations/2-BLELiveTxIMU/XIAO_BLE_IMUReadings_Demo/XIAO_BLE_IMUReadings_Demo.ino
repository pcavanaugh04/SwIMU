/*****************************************************************************/
/*  
  XIAO_BLE_IMUReadings_Demo.ino
  Hardware:      Seeed XIAO BLE Sense - nRF52840
	Arduino IDE:   Arduino-2.3.4
	Author:	       pcavanaugh04
	Date: 	       Dec,2024
	Version:       v1.0

  This tutorial is the 2nd part of the communication module in the tutorial 
  for my swIMU project. This series is aimed to teach some of the
  fundamentals of product design in sports technology applications, including
  user interfaces, technical interfaces, and technical implementations. 

  If you are just getting started... Go back to the first example!
  
  This tutorial extends our introduction to BLE communication by configuring
  out client and server to devices to stream real-time IMU data via BLE.
  This code configures the server device to accept a start command, read
  data and write it to a custom characteristic until we recieve the 
  end command. The client script "client_BLE_IMUReadings_Demo.py" recieves
  these notificaitons with different methods. 

  Note, BLE is not the best way to accomplish this, but it's what's avialable
  on the XIAO BLE Sense device. BLE is limited in bandwidth and not suitable
  for continuous transfer. But it's good enough to demonstrate some of the
  features
  
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

#include <ArduinoBLE.h>
#include <LSM6DS3.h>
#include <string.h>

int currentCount;

// BLE Setup
const char * IMUServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
// Ensure First block of characteristic UUID is incremented +1 of Service UUID
const char * IMURequestCharacteristicUuid = "550e8401-e29b-41d4-a716-446655440001";
const char * IMUDataCharacteristicUuid = "550e8401-e29b-41d4-a716-446655440002";

// Accelerometer Variables
unsigned long accelStartMillis;
unsigned long numSamples = 0;
float accelTime;
bool TxActive = false;
//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A


// These are BLE services and characteristics. A service is a group of characteristics. 
// Characteristics are the endpoints between client and server where data is written to/from
// BLE protocol defines a number of standard services and characteristics, which are 
// noted in the official documentation. Users can configure their own custom services and
// Characteristics, but they need to create their own UUIDs
BLEService imuService(IMUServiceUuid);
BLEStringCharacteristic imuRequestCharacteristic(IMURequestCharacteristicUuid, BLEWrite, 10);
BLEStringCharacteristic imuDataCharacteristic(IMUDataCharacteristicUuid, BLERead | BLENotify, 512);


// User defined function for reading accellerometer data
const char* readAccel() {
// Read a line of values from the accelerometer
  // Calculate elapsed time
  accelTime = (float)(millis() - accelStartMillis) / 1000;
  // Create a buffer to collect the values to transmit
  static char accelBuffer[100];
  
  // Read Accelerometer values
  float accelX = myIMU.readFloatAccelX();
  float accelY = myIMU.readFloatAccelY();
  float accelZ = myIMU.readFloatAccelZ();
  float gyroX = myIMU.readFloatGyroX();
  float gyroY = myIMU.readFloatGyroY();
  float gyroZ = myIMU.readFloatGyroZ();
  sprintf(accelBuffer, "%.5f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f", accelTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
  // Increment a counter to track data rate
  numSamples++;
  // Serial.println(accelBuffer);
  return accelBuffer;
}

void setup() {
  
  // Accelerometer Init
  if (myIMU.begin() != 0) {
    Serial.println("Device error");
  } 
  else {
    Serial.println("Device OK!");
  }

  // Configure discovery characteristics, this allows us to recongize the device by name
  BLE.setDeviceName("SwIMU");
  BLE.setLocalName("SwIMU");

  if(!BLE.begin()) {
    Serial.println("- Starting BLE Module Failed!");
    while(1);
  }

  // broadcast service to central devices for connections
  imuService.addCharacteristic(imuRequestCharacteristic);
  imuService.addCharacteristic(imuDataCharacteristic);
  BLE.addService(imuService);
  BLE.setAdvertisedService(imuService);
  BLE.advertise();
  Serial.println("BLE device ready to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  Serial.println("Discovering Central device");
  delay(500);
  
  // Upon connection, index a start time before entering the broadcast loop
  if (central.connected())
  {
    accelStartMillis = millis();
    Serial.println("New Value of Start Millis: ");
    Serial.println(accelStartMillis);
    Serial.println("Central Connected! Awaiting data request");
    // Delay in connection to allow central time to config services
    delay(100);
  }

  while (central.connected()) {
    // start each loop with a check to the response characteristic to see if
    // weve recieved new instructions from the client
    if (imuRequestCharacteristic.written()) {
      String request = String(imuRequestCharacteristic.value());
      Serial.println("Request Recieved: " + request);
      // If we've recieved a start command, switch the flag to true
      if (request.equals("START")) {
        TxActive = true;
        numSamples = 0;
      }

      // If we recieve a stop command, switch command to false and calc data rate
      // of the session
      else {
        TxActive = false;
        float freq = numSamples/accelTime;
        Serial.println("Realized Data Frequency [Hz]: " + String(freq));
      }
    }

    if(TxActive == true) {
      const char* newValues = readAccel();
      // Serial.print("Value from readAccel: " + newValues);
      imuDataCharacteristic.setValue(newValues);
    }
  }
}
