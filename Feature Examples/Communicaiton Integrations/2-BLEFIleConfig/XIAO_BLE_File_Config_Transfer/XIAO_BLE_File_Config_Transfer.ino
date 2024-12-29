/*****************************************************************************/
/*  
  XIAO_BLE_Config.ino
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


#include <SPI.h>
#include <SD.h>
#include <ArduinoBLE.h>
#include <LSM6DS3.h>
#include <string.h>

// SD Card Setup
const int chipSelect = 5;
int currentCount;
bool fileConfigMode = false;
bool fileTxMode = true;
String dateTime;
String swimmerName;
File dataFile;

// BLE Setup
// Service to config device for data recording
const char * fileConfigServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
// Ensure First block of characteristic UUID is incremented +1 of Service UUID
const char * fileConfigDateTimeCharUuid = "550e8401-e29b-41d4-a716-446655440001";
const char * fileConfigNameCharUuid = "550e8401-e29b-41d4-a716-446655440002";

// Service to transfer file data
const char * fileTransferServiceUuid = "550e8402-e29b-41d4-a716-446655440000";
// Ensure First block of characteristic UUID is incremented +1 of Service UUID
const char * fileTransferRequestCharUuid = "550e8403-e29b-41d4-a716-446655440001";
const char * fileTransferDataCharUuid = "550e8403-e29b-41d4-a716-446655440002";
const char * fileTransferCompleteCharUuid = "550e8403-e29b-41d4-a716-446655440003";
const char * fileNameCharUuid = "550e8403-e29b-41d4-a716-446655440004";

// Experiment with buffer size of file tx chunks
const int fileTxBufferSize = 244; // Oddly enough this seems to be the bandwidth of the string Char
const bool fixedLength = false;
// Accelerometer Variables
unsigned long accelStartMillis;
unsigned long numSamples = 0;
float accelTime;
//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A

BLEService fileConfigService(fileConfigServiceUuid);
BLEStringCharacteristic fileConfigDateTimeChar(fileConfigDateTimeCharUuid, BLERead | BLEWrite, 20);
BLEStringCharacteristic fileConfigNameChar(fileConfigNameCharUuid, BLEWrite, 20);

BLEService fileTransferService(fileTransferServiceUuid);
BLEStringCharacteristic fileTransferRequestChar(fileTransferRequestCharUuid, BLEWrite, 10);
BLEStringCharacteristic fileNameChar(fileNameCharUuid, BLERead | BLENotify, 50);
BLECharacteristic fileTransferDataChar(fileTransferDataCharUuid, BLENotify, fileTxBufferSize, fixedLength);
BLEStringCharacteristic fileTransferCompleteChar(fileTransferCompleteCharUuid, BLENotify, 30);

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

String readAccel() {

      // Serial.println("Entering Read Accel Method");
      accelTime = (float)(millis() - accelStartMillis) / 1000;
      //Serial.println("Past accelTime Calculation");
      char accelBuffer[100];
      float accelX = myIMU.readFloatAccelX();
      // Serial.println("Read aX");
      float accelY = myIMU.readFloatAccelY();
      // Serial.println("Read aY");
      float accelZ = myIMU.readFloatAccelZ();
      // Serial.println("Read aZ");
      float gyroX = myIMU.readFloatGyroX();
      // Serial.println("Read gX");
      float gyroY = myIMU.readFloatGyroY();
      // Serial.println("Read gY");
      float gyroZ = myIMU.readFloatGyroZ();
      // Serial.println("Read gZ");
      sprintf(accelBuffer, "%.5f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f", accelTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
      // Serial.print("Constructed Accel Buffer: ");
      // Serial.println(accelBuffer);
      // Serial.print("With size of: ");
      // Serial.println(sizeof(accelBuffer));

      // Serial.print("From inside readAccel function: ");
      // Serial.println(accelBuffer);
      return String(accelBuffer);
      // Serial.println(accelBuffer);
      // accelDataFile.println(accelBuffer);
      // numSamples = numSamples + 6;
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
  
  // Read contents of counter file to determine most recent file
  File counterFileContents = SD.open("accelDir/counter.txt", FILE_READ);
  if (counterFileContents) {
    currentCount = counterFileContents.parseInt();
    counterFileContents.close();
  }
  
  else Serial.println("Error opening Counter File!");
  
  // Accelerometer Init
  if (myIMU.begin() != 0) {
    Serial.println("Device error");
  } 
  else {
    Serial.println("Device OK!");
  }

  BLE.setDeviceName("SwIMU");
  BLE.setLocalName("SwIMU");

  if(!BLE.begin()) {
    Serial.println("- Starting BLE Module Failed!");
    while(1);
  }

  // Add characteristics to services, add services to device
  fileConfigService.addCharacteristic(fileConfigDateTimeChar);
  fileConfigService.addCharacteristic(fileConfigNameChar);
  BLE.addService(fileConfigService);

  fileTransferService.addCharacteristic(fileTransferRequestChar);
  fileTransferService.addCharacteristic(fileTransferDataChar);
  fileTransferService.addCharacteristic(fileNameChar);
  fileTransferService.addCharacteristic(fileTransferCompleteChar);
  BLE.addService(fileTransferService);

  // Advertise file config service
  BLE.setAdvertisedService(fileConfigService);

  fileConfigDateTimeChar.writeValue("90");
  BLE.advertise();
  Serial.println("BLE device ready to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  // Serial.println("Discovering Central device");
  delay(500);
  
  // Upon connection, index a start time before entering the broadcast loop
  if (central.connected())
  {
    accelStartMillis = millis();
    Serial.println("New Value of Start Millis: ");
    Serial.println(accelStartMillis);
    // Delay in connection to allow central time to config services
    delay(1);
  }
  // String newValues = readAccel();
  // Serial.print("New Readings from IMU: ");
  // Serial.println(newValues);
  while (central.connected() && (fileConfigMode == true)) {
    // poll until both name and datetime Chars have been written to
    if (fileConfigDateTimeChar.written()) {
      dateTime = String(fileConfigDateTimeChar.value());
      Serial.print("Received DateTime Value: ");
      Serial.println(dateTime);
    }

    if (fileConfigNameChar.written()) {
      swimmerName = String(fileConfigNameChar.value());
      Serial.print("Received Name Value: ");
      Serial.println(swimmerName);
    }

    if ((dateTime.length() > 0) && (swimmerName.length() > 0)) {
      char fileNameBuffer[100];
      sprintf(fileNameBuffer, "%s-%s.csv", dateTime.c_str(), swimmerName.c_str());
      Serial.println(fileNameBuffer);
      // Disconnect from central and exit config loop 
      central.disconnect();
      fileConfigMode = false;
      fileTxMode = true;

    }

    else {
      Serial.println("Connected and Awaiting dateTime and swimmerName Variables.");
    }

    delay(500);
  }

  if (fileTxMode == true) {
    BLE.setAdvertisedService(fileTransferService);
    // fileTransferDataChar.writeValue("Ready to Transmit");
    BLE.advertise();
  }

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



