/*****************************************************************************/
/*  
  XIAO_BLE_Config.ino
  Hardware:      Seeed XIAO BLE Sense - nRF52840
	Arduino IDE:   Arduino-2.3.4
	Author:	       pcavanaugh04
	Date: 	       Dec,2024
	Version:       v1.0

  This tutorial is the 1st part of the communication module in the tutorial 
  for my swIMU project. This series is aimed to teach some of the
  fundamentals of product design in sports technology applications, including
  user interfaces, technical interfaces, and technical implementations. 

  If you are just getting started... Go back to the first example!
  
  This tutorial introduces the Bluetooth Low Energy (BLE) protocol with a
  script to create a connection between a client and server device, and
  transmit configuration info from the client to the server

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

// Include libraries
#include <ArduinoBLE.h>
#include <LSM6DS3.h>
#include <string.h>

// Declare the variables we want to recieve from the client
String dateTime;
String personName;
String activityType;
String fileName;
unsigned long dateTimeRefrenceMillis;

// I'm using these variables because its a good way to name a file when
// we get to recording data

// BLE Setup
// This implementation of BLE is composed of services and characteristics.
// Services are collections of characteristics, and characteristics are points
// that can be read/written from the client and server. We need to define 
// universally unique identifiers (UUID) for each service/characteristic 

const char* configInfoServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
// Ensure First block of characteristic UUID is incremented +1 of Service UUID
const char* dateTimeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440001";
const char* personNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440002";
const char* activityTypeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440003";
const char* fileNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440004";


// Define our service and characteristics
BLEService configInfoService(configInfoServiceUuid);

// Characteristics can be defined a number of ways, in this instance, we've configed
// a "String characteristic" to transfer string data back and forth, the three arguments 
// of the constructor are (UUID, Property, Size), which define the UUID of the charactteristic,
// how the characteristic will be used, and the size [in bytes] of the info communicated.
// Check out the documentation for ArduinoBLE for more config options
BLEStringCharacteristic dateTimeConfigChar(dateTimeConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic personNameConfigChar(personNameConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic activityTypeConfigChar(activityTypeConfigCharUuid, BLEWrite, 25);
BLEStringCharacteristic fileNameConfigChar(fileNameConfigCharUuid, BLERead, 80);

void incrementDateTime(String dateTime, int newSeconds, String &result) {
  // Function to return the number of seconds since the start of a given day based on a given
  // datetime string format "%Y_%m_%d_%H_%M_%S"
  int year, month, day, hours, minutes, seconds;

  // Parse the datetime string using sscanf
  int parsed = sscanf(dateTime.c_str(), "%d_%d_%d_%d_%d_%d", &year, &month, &day, &hours, &minutes, &seconds);

  // Check if parsing succeeded (6 items should be matched)
  if (parsed != 6) {
    Serial.println("Error: Invalid datetime format.");
    result = "ERROR!";
    return;
  }

  // Calculate new values for hours, minutes and seconds based on the number of new seconds
  int newTotalSeconds = (hours * 3600) + (minutes * 60) + seconds + newSeconds;
  hours = newTotalSeconds / 3600;
  minutes = newTotalSeconds % 3600 / 60;
  seconds = newTotalSeconds % 3600 % 60;
  char buffer[25];
  sprintf(buffer, "%d_%d_%d_%d_%d_%d", year, month, day, hours, minutes, seconds);
  result = String(buffer);
}

String bytesToString(byte* data, int length) {
  char charArray[length + 1];
  for (int i=0; i < length; i++) {
    charArray[i] = (char)data[i];
  }
  charArray[length] = '\0';
  String finalString = String(charArray);
  return finalString;
}

void onDateTimeCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  dateTime = bytesToString(data, length);
  dateTimeRefrenceMillis = millis();
  Serial.println("New DateTime Characteristic Recieved: " + dateTime);
  fileName = dateTime + "-" + personName + "-" + activityType;
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);
}

void onPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  personName = bytesToString(data, length); 
  Serial.println("New personName Characteristic Recieved: " + personName);
  fileName = dateTime + "-" + personName + "-" + activityType;
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);
}

void onActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  activityType = bytesToString(data, length);
  Serial.println("New activityType Characteristic Recieved: " + activityType);
  fileName = dateTime + "-" + personName + "-" + activityType;
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Test?");

  // Set the device name of the BLE module
  BLE.setDeviceName("SwIMU");
  BLE.setLocalName("SwIMU");

  if(!BLE.begin()) {
    Serial.println("- Starting BLE Module Failed!");
    while(1);
  }

  // Add characteristics to services, add services to device
  configInfoService.addCharacteristic(dateTimeConfigChar);
  configInfoService.addCharacteristic(personNameConfigChar);
  configInfoService.addCharacteristic(activityTypeConfigChar);
  configInfoService.addCharacteristic(fileNameConfigChar);
  dateTimeConfigChar.setEventHandler(BLEWritten, onDateTimeCharWritten);
  personNameConfigChar.setEventHandler(BLEWritten, onPersonNameCharWritten);
  activityTypeConfigChar.setEventHandler(BLEWritten, onActivityTypeCharWritten);
  BLE.addService(configInfoService);

  // Advertise file config service for discovery
  BLE.setAdvertisedService(configInfoService);
  BLE.advertise();
  Serial.println("BLE device ready to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  // This scans for central devices to connect to. The central will be a computer running
  // the accompanying python script
  BLEDevice central = BLE.central();
  // Serial.println("Discovering Central device");
  delay(500);
  
  // Once connected, the client will write values to our characteristics
  while (central.connected()) {
    // We can use the information information to create a date Time index for our device.
    // Because we dont have the capability to generate a datetime value from the device,
    // we can use this value as a baseline and increment it with millis when we want 
    // an updated timestamp
    BLE.poll();
    delay(1000);
    if (dateTimeConfigChar.written()) {
    int incrementSeconds = 10000;
    String newDateTime;
    incrementDateTime(dateTime, incrementSeconds, newDateTime);
    Serial.print("New refrence Value for DateTime: ");
    Serial.println(newDateTime);
    }
  }
}