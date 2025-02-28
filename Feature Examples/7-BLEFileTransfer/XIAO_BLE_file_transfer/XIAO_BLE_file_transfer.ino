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
  
  This tutorial continues our introduction to BLE communication by setting up a file
  exchange between our server and client. We'll read an example file generated 
  during the hardware tutorials 1-4 thats stored on our sd card, transmit the file
  name, and file contents to the client.

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

#include <SPI.h>
#include <SdFat.h>
#include <ArduinoBLE.h>
#include <string.h>

// SD Card Setup
const int chipSelect = 5;
int currentCount;
bool fileConfigMode = false;
bool fileTxMode = false;
String dataFileName;
SdFat32 sd;
// BLE Setup

// Service to transfer file data
const char * fileTransferServiceUuid = "550e8402-e29b-41d4-a716-446655440000";
// Associated characteristics to read/write from
const char * fileTransferRequestCharUuid = "550e8403-e29b-41d4-a716-446655440001";
const char * fileTransferDataCharUuid = "550e8403-e29b-41d4-a716-446655440002";
const char * fileTransferCompleteCharUuid = "550e8403-e29b-41d4-a716-446655440003";
const char * fileNameResponseCharUuid = "550e8403-e29b-41d4-a716-446655440004";

// Experiment with buffer size of file tx chunks
// Data is transmitted sequentially over BLE, and is limited by the packet size config
// of the device and client, through trial and error, I found out the max size was 244
// bytes. Weird.
const int fileTxBufferSize = 244; // Oddly enough this seems to be the bandwidth of the string Char

BLEService fileTransferService(fileTransferServiceUuid);
// We'll use this characteristic to accept requests and respond to the request with the file name
BLEStringCharacteristic fileTransferRequestChar(fileTransferRequestCharUuid, BLEWrite, 10);
// Characteristic to be used to send the data
BLECharacteristic fileTransferDataChar(fileTransferDataCharUuid, BLENotify, fileTxBufferSize, false);
// Transfer complete notification to signal to the client to move on
BLEStringCharacteristic fileTransferCompleteChar(fileTransferCompleteCharUuid, BLENotify, 30);
BLEStringCharacteristic fileNameResponseChar(fileNameResponseCharUuid, BLERead, 60);

String bytesToString(byte* data, int length) {
  char charArray[length + 1];
  for (int i=0; i < length; i++) {
    charArray[i] = (char)data[i];
  }
  charArray[length] = '\0';
  String finalString = String(charArray);
  return finalString;
}


void findFileToTx() {
  // For now we'll identify and transmit the latest file in the accelDir directory 
  /*
  File32 accelDir =  SD.open("/accelDir");
  unsigned long latestTimeStamp = 0;
  fileTxMode = false;
  if (!accelDir || !accelDir.isDirectory()) {
      Serial.println("Directory not found or error opening it.");
      return;
    }
  
  // Cycle through the directory and identify the most recent timestamp from metadata
  
  while (true) {
    File entry = accelDir.openNextFile();
    if (!entry) break; // No more files

    if (!entry.isDirectory() && !(entry.name() == "counter.txt")) {
      dataFileName = entry.name(); // Assign the new most recent file
      entry.close();
    }
    accelDir.close();
  }
  */
  dataFileName = "2025_02_10_12_19_17-justin-test.csv";
  if (dataFileName.length() > 0) {
    fileNameResponseChar.writeValue(dataFileName);
  }
}


void transmitFileData() {
  // Open the specified file
  String filePath = "accelDir/" + dataFileName;
  Serial.print("Attempting to open file: ");
  Serial.println(filePath);
        // Check to see if file exists
  if (!sd.exists(filePath)){
    Serial.println("ERROR: File not recognized!");
    return;
  }
  
  File32 dataFile;
  dataFile.open(filePath.c_str(), O_READ);
  char buf[100];
  Serial.println(dataFile.getName(buf, 100));
  // Create a time counter for funsies
  int txStartTime = millis();
  Serial.println("File opened. Transmitting...");
  // Parse through the datafile, chunking data into payloads and transmitting
  // each payload sequentially
  while (dataFile.available()) {
    char buffer[fileTxBufferSize]; // Create a buffer to fill with data
    int bytesRead = dataFile.read(buffer, fileTxBufferSize);
    Serial.print(buffer);
    // Write the full buffer to the characteristic
    fileTransferDataChar.writeValue((uint8_t*)buffer, bytesRead);
    delay(30); // Small delay to prevent BLE stack overflow

  }
  int txElapsedTime = millis() - txStartTime;
  // Send a notification when the end is reached
  fileTransferCompleteChar.writeValue("TRANSFER_COMPLETE");
  Serial.print("File transmission completed in:");
  Serial.print(txElapsedTime);

  // Close the file
  dataFile.close();
}


void onFileTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  String fileTxRequest = bytesToString(data, length);
  
  Serial.println("Request Recieved: " + fileTxRequest);
  if (fileTxRequest.equals("SEND_FILE")) {
    findFileToTx();
  }
  
  // If "Start" is written to the charactyeristic, then the client is ready to recieve data
  else if (fileTxRequest.equals("START")) {
    transmitFileData();
  }
}
// From previous tutorials, useful to debug our SD card contents
void displayDirectory(const char* dirName, int numTabs=0) {
  File32 dir;
  if (!dir.open(dirName, O_RDONLY)) {
    Serial.print("Failed to open directory: ");
    Serial.println(dirName);
    return;
  }

  File32 entry;
  while (entry.openNext(&dir, O_RDONLY)) {
        entry = dir.openNextFile();
        if (!entry) break; // No more files

        for (int i = 0; i < numTabs; i++) {
            Serial.print("\t");
        }
        char buf[100];
        entry.getName(buf, sizeof(buf));
        Serial.print(buf);
        if (entry.isDir()) {
            Serial.println("/");

            displayDirectory(buf, numTabs + 1); // Recursive call for subdirectories
        } else {
            Serial.print("\t\t");
            Serial.println(entry.fileSize());
        }
        entry.close();
    }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);
  digitalWrite(chipSelect, HIGH);
  Serial.println("Initializing SD Card...");
  delay(1000);

  if (!sd.begin(chipSelect, SD_SCK_MHZ(25))) {
    Serial.println("Initializing failed!");
    while (1);
  }

  else {
    Serial.println("Initialization done.");
  }

  displayDirectory("accelDir");
  
  BLE.setDeviceName("SwIMU");
  BLE.setLocalName("SwIMU");

  if(!BLE.begin()) {
    Serial.println("- Starting BLE Module Failed!");
    while(1);
  }

  // Add characteristics to services, add services to device
  fileTransferService.addCharacteristic(fileTransferRequestChar);
  fileTransferService.addCharacteristic(fileTransferDataChar);
  fileTransferService.addCharacteristic(fileTransferCompleteChar);
  fileTransferService.addCharacteristic(fileNameResponseChar);
  BLE.addService(fileTransferService);

  // Assign Callback
  fileTransferRequestChar.setEventHandler(BLEWritten, onFileTxRequest);

  // Advertise file config service
  BLE.advertise();
  Serial.println("BLE device ready to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  // Serial.println("Discovering Central device");
  delay(500);
  
  while (central.connected()) {
    // Since all our code is callback based, not alot going on here
    BLE.poll();      
  }
}



