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
#include <SD.h>
#include <ArduinoBLE.h>
#include <string.h>

// SD Card Setup
const int chipSelect = 5;
int currentCount;
bool fileConfigMode = false;
bool fileTxMode = false;
String dataFileName;
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

void onFileTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  String fileTxRequest = bytesToString(data, length);
  
  Serial.println("Request Recieved: " + fileTxRequest);
  // If we've recieved a start command, switch the flag to true
  if (fileTxRequest.equals("SEND_FILE")) {
    // For now we'll identify and transmit the latest file in the accelDir directory 
    File accelDir =  SD.open("/accelDir");
    String dataFileName;
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
    if (dataFileName.length() > 0) {
      fileNameResponseChar.writeValue(dataFileName);
    }
  }
  
  else if (fileTxRequest.equals("START")) {
    File dataFile = SD.open("/accelDir/" + dataFileName);
    int txStartTime = millis();
    Serial.println("File opened. Transmitting...");
    while (dataFile.available()) {
      char buffer[fileTxBufferSize]; // BLE max payload size is typically 512 bytes
      int bytesRead = dataFile.read(buffer, fileTxBufferSize);
      // Convert buffer into a byte array
      // byte byteBuffer[bytesRead] = {byte(atoi(buffer))};
      // String bufferStr = String(buffer);
      // Serial.print("Contents of Buffer: ");
      // Serial.println(buffer);
      // Send the data over BLE
      // fileTransferDataChar.writeValue("Test Buffer Line");
      // String transferBuffer = buffer;
      // Serial.print(buffer);
      fileTransferDataChar.writeValue((uint*)buffer, bytesRead);

      delay(30); // Small delay to prevent BLE stack overflow
      // Serial.print("Loop time [ms]: ");
      // Serial.println(loopTime);
      // break; // To try only one notificaiton
    }
    int txElapsedTime = millis() - txStartTime;
    fileTransferCompleteChar.writeValue("TRANSFER_COMPLETE");
    Serial.print("File transmission completed in:");
    Serial.print(txElapsedTime);

    // Close the file and disconnect
    dataFile.close();
  }


}
// From previous tutorials, useful to debug our SD card contents
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);
  digitalWrite(chipSelect, HIGH);
  Serial.println("Initializing SD Card...");
  delay(1000);

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
  
  // Upon connection, index a start time before entering the broadcast loop
  // String newValues = readAccel();
  // Serial.print("New Readings from IMU: ");
  // Serial.println(newValues);
  while (central.connected()) {
    BLE.poll();

 /*
  while ((central.connected()) && (fileTxMode == true)){
    // Delay 10 seconds to simulate data collection
    // Reconnect to the central, this would be done after entering fileTxMode in real time

      // Procedure follows a handshake between the client and server to ensure file
      // data is available and configured correctly on both sides of the transmission

      // Grab the most recent file on the sd card
      if (fileTransferRequestChar.written()) {
        String value = String(fileTransferRequestChar.value().c_str());
        Serial.print("Recieved Request Value: ");
        Serial.println(value);
        if (value == "Send_File") {
          char dataFileName[30];
          String rootFolder = "accelDir";
          sprintf(dataFileName, "%s/DATA_%d.CSV", rootFolder.c_str(), currentCount);
          Serial.print("Attempting to open file: ");
          Serial.println(dataFileName);
          // Check to see if file exists
          if (SD.exists(dataFileName)) { //check for existing or duplicate file names
            Serial.println("File exists");
            fileNameChar.writeValue(dataFileName);
          }

          dataFile = SD.open(dataFileName, FILE_READ);
          if (!dataFile) {
            Serial.println("Failed to open file!");
            fileNameChar.writeValue("ERROR");
            central.disconnect();
            return;
          }
        }
        else if (value == "START") {
          while (dataFile.available()) {
            // Serial.println("File opened. Transmitting...");
            int startTime = millis();
            char buffer[fileTxBufferSize]; // BLE max payload size is typically 512 bytes
            int bytesRead = dataFile.read(buffer, fileTxBufferSize);
            // Convert buffer into a byte array
            // byte byteBuffer[bytesRead] = {byte(atoi(buffer))};
            // String bufferStr = String(buffer);
            // Serial.print("Contents of Buffer: ");
            // Serial.println(buffer);
            // Send the data over BLE
            // fileTransferDataChar.writeValue("Test Buffer Line");
            // String transferBuffer = buffer;
            // Serial.print(buffer);
            fileTransferDataChar.writeValue((uint*)buffer, bytesRead);

            delay(30); // Small delay to prevent BLE stack overflow
            int loopTime = startTime - millis();
            // Serial.print("Loop time [ms]: ");
            // Serial.println(loopTime);
            // break; // To try only one notificaiton
          }
          fileTransferCompleteChar.writeValue("TRANSFER_COMPLETE");
          Serial.println("File transmission completed.");

          // Close the file and disconnect
          dataFile.close();
          // Give time for client to process data
          delay(5000);
          central.disconnect();
          Serial.println("Central disconnected.");
          fileTxMode = false;
          break;
      }
      */
      // Await request from central:
      // Read file contents and send via BLE
      /*
      if (fileNameChar.written()) {
        String value = String(fileNameChar.value().c_str());
        if (value == "ACK") {
          while (dataFile.available()) {
            Serial.println("File opened. Transmitting...");
            char buffer[512]; // BLE max payload size is typically 512 bytes
            size_t bytesRead = dataFile.readBytes(buffer, sizeof(buffer));

            // Send the data over BLE
            fileTransferDataChar.writeValue(buffer);

            delay(20); // Small delay to prevent BLE stack overflow
          }
          fileTransferCompleteChar.writeValue("TRANSFER_COMPLETE");
          Serial.println("File transmission completed.");

          // Close the file and disconnect
          dataFile.close();
          central.disconnect();
          Serial.println("Central disconnected.");
          fileTxMode = false;
          break;
        }
      */
      
    }
  }
    // Further logic
      // Keep loop going until we've recieved both pieces of information
      // Once we've recieved both, config a new file name, indicate somehow that
      // file has been configured and ready
      // Disconnect Device

    // Simulate logic to indicate device is in file transfer mode
      // BLE goes to connect again with advertised fileTxService
      // Device sends request to transfer with file name
      // Central confirms readiness to recieve
      // Device commensses file transfer
      // Device notifies that file transfer is complete
      // Central saves and closes file


    // If we want to respond to a query, use this if structure a reading from the sensor
    // if (imuResponseCharacteristic.written()) {}
    // String newValues = readAccel();
    // String newValues = readAccel();
    // Serial.print("Value from readAccel: ");
    // Serial.println(newValues);
    // imuResponseCharacteristic.setValue(newValues);



