//========================================================//
/* Header File for BLE Manager for SwIMU project
This library manages all the BLE functions of the board, namely
configuring with the client, realtime data transmission, and 
file transmission

Contains:
  Classes:
    BLEManager
  
  Functions
config name
*/
//==========================================================//


// Module Goals:
// Get definitions for all prototypes in .h file
// Enable File Config with Client
// Enable live Tx and data record
// Enable file search and transfer


#include "BLEManager.h"

const int fileTxBufferSize = 244; // Oddly enough 244 bytes seems to be the bandwidth of the BLE char

static std::map<const char*, BLEManager*> characteristicToInstanceMap;

String bytesToString(byte* data, const int length) {
  char charArray[length + 1];
  for (int i=0; i < length; i++) {
    charArray[i] = (char)data[i];
  }
  charArray[length] = '\0';
  return String(charArray);
}

static void staticOnIMURequest(BLEDevice central, BLECharacteristic characteristic) {
  BLEManager* instance = characteristicToInstanceMap[characteristic.uuid()];
  if (instance) {
      instance->onIMUTxRequest(central, characteristic);
  }
}

static void staticOnDateTimeCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  BLEManager* instance = characteristicToInstanceMap[characteristic.uuid()];
  if (instance) {
      instance->onDateTimeCharWritten(central, characteristic);
  }  
}

static void staticOnPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  Serial.println("Recieved Callback to Static Person Name!");
  Serial.print("Value of characteristic UUID: ");
  Serial.println(characteristic.uuid());
  Serial.print("Value of characteristic Address in Callback: ");
  Serial.println((uintptr_t)&characteristic, HEX);
    BLEManager* instance = characteristicToInstanceMap[characteristic.uuid()];
  Serial.print("Value of instance:");
  Serial.println(bool(instance));
  if (instance) {
      instance->onPersonNameCharWritten(central, characteristic);
  }  
}

static void staticOnActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  BLEManager* instance = characteristicToInstanceMap[characteristic.uuid()];
  if (instance) {
      instance->onActivityTypeCharWritten(central, characteristic);
  }  
}

static void staticOnFileTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  BLEManager* instance = characteristicToInstanceMap[characteristic.uuid()];
  if (instance) {
      instance->onFileTxRequest(central, characteristic);
  }  
}
/*
static void staticOnConnect(BLEDevice central) {
  BLEManager* instance = static_cast<BLEManager*>(characteristic.userContext());
  if (instance) {
      instance->onConnect(central);
  }  
}

static void staticOnDisconnect(BLEDevice central) {
  BLEManager* instance = static_cast<BLEManager*>(characteristic.userContext());
  if (instance) {
      instance->onDisconnect(central);
  }
}

*/
BLEManager::BLEManager(DataRecorder& dataRecorder): dataRecorder(dataRecorder), // Initialize DataRecorder reference

      // Initialize BLE Configuration Service and Characteristics
      configInfoService(configInfoServiceUuid),
      dateTimeConfigChar(dateTimeConfigCharUuid, BLEWrite, 25),
      personNameConfigChar(personNameConfigCharUuid, BLEWrite, 25),
      activityTypeConfigChar(activityTypeConfigCharUuid, BLEWrite, 25),
      fileNameConfigChar(fileNameConfigCharUuid, BLERead, 80),

      // Initialize BLE IMU Transfer Service and Characteristics
      imuTxService(IMUServiceUuid),
      imuRequestChar(IMURequestCharUuid, BLEWrite, 10),
      imuDataChar(IMUDataCharUuid, BLERead | BLENotify, 100),

      // Initialize BLE File Transfer Service and Characteristics
      fileTxService(fileTxServiceUuid),
      fileTxRequestChar(fileTxRequestCharUuid, BLEWrite | BLERead, 20),
      fileTxCompleteChar(fileTxCompleteCharUuid, BLENotify, 30),
      fileTxDataChar(fileTxDataCharUuid, BLERead | BLENotify, fileTxBufferSize, false),
      fileNameTxChar(fileNameTxCharUuid, BLERead, 60) {

  // Needs to be defined in the body due to needing value of fileTxBufferSize
 
  // Map characteristics that will be used for event handlers
  characteristicToInstanceMap[imuRequestChar.uuid()] = this;
  characteristicToInstanceMap[dateTimeConfigChar.uuid()] = this;
  characteristicToInstanceMap[personNameConfigChar.uuid()] = this;
  characteristicToInstanceMap[activityTypeConfigChar.uuid()] = this;
  characteristicToInstanceMap[fileTxRequestChar.uuid()] = this;
  // Keep these open for onConnect and onDisconnect;
  // characteristicToInstanceMap[&imuRequestChar] = this;
  // characteristicToInstanceMap[&imuRequestChar] = this;

  // Config Service
  configInfoService.addCharacteristic(dateTimeConfigChar);
  configInfoService.addCharacteristic(personNameConfigChar);
  configInfoService.addCharacteristic(activityTypeConfigChar);
  configInfoService.addCharacteristic(fileNameConfigChar);
  dateTimeConfigChar.setEventHandler(BLEWritten, staticOnDateTimeCharWritten);
  personNameConfigChar.setEventHandler(BLEWritten, staticOnPersonNameCharWritten);
  activityTypeConfigChar.setEventHandler(BLEWritten, staticOnActivityTypeCharWritten);

  // IMU Transfer Service
  imuTxService.addCharacteristic(imuRequestChar);
  imuTxService.addCharacteristic(imuDataChar);
  imuRequestChar.setEventHandler(BLEWritten, staticOnIMURequest);

  // File Transfer Service
  fileTxService.addCharacteristic(fileTxRequestChar);
  fileTxService.addCharacteristic(fileTxDataChar);
  fileTxService.addCharacteristic(fileTxCompleteChar);
  fileTxService.addCharacteristic(fileNameTxChar);
  fileTxRequestChar.setEventHandler(BLEWritten, staticOnFileTxRequest);

  // Connection Handlers
}


void BLEManager::startBLE() {
  // Function to be called in setup
  BLE.setLocalName(deviceName);
  BLE.setDeviceName(deviceName);
  if(!BLE.begin()) {
    Serial.println("- Starting BLE Module Failed!");
    while(1);
  }
  
  Serial.print("Value of characteristic Address in Map definition: ");
  Serial.println((uintptr_t)&personNameConfigChar, HEX);

  BLE.addService(configInfoService);
  BLE.addService(imuTxService);
  BLE.addService(fileTxService);
  BLE.setAdvertisedService(configInfoService);
  // Set event handlers
  // BLE.setEventHandler(BLEConnected, staticOnConnect);
  // BLE.setEventHandler(BLEDisconnected, staticOnDisconnect);
  // BLE.advertise();
  Serial.println("BLE device ready to connect");

}


// ------------------ Getters and Setters -------------------- //

String BLEManager::updateFileName() {
  dataFileName = (getDateTimeStr() + "-" + personName + "-" + activityType + ".csv");
  return dataFileName;
}

void BLEManager::poll() {
  BLE.poll();
}

String BLEManager::getDateTimeStr() {
   // Function to return the number of seconds since the start of a given day based on a given
  // datetime string format "%Y_%m_%d_%H_%M_%S"
  int year, month, day, hours, minutes, seconds;

  // Parse the datetime string using sscanf
  int parsed = sscanf(dateTimeStr.c_str(), "%d_%d_%d_%d_%d_%d", &year, &month, &day, &hours, &minutes, &seconds);

  // Check if parsing succeeded (6 items should be matched)
  if (parsed != 6) {
    Serial.print("Error: Invalid datetime format. Number of fields recognized: ");
    Serial.println(parsed);
    Serial.print("Contents of dateTimeStr variable: ");
    Serial.println(dateTimeStr);
    return "ERROR";
  }

  // Calculate new values for hours, minutes and seconds based on the number of new seconds
  unsigned long newSeconds = (millis() - dateTimeRefrenceMillis) / 1000;
  int newTotalSeconds = (hours * 3600) + (minutes * 60) + seconds + newSeconds;
  hours = newTotalSeconds / 3600;
  minutes = newTotalSeconds % 3600 / 60;
  seconds = newTotalSeconds % 3600 % 60;
  char buffer[25];
  sprintf(buffer, "%04d_%02d_%02d_%02d_%02d_%02d", year, month, day, hours, minutes, seconds);
  dateTimeStr = String(buffer);
  return dateTimeStr;
}

void BLEManager::enterPairingMode(int timeout) {
  timeoutStart = millis();
  timeoutDuration = timeout;
  inPairingMode = true;
  Serial.println("Starting Pairing Mode!");
  reachedTimeout = false;
}

void BLEManager::pairCentral() {
  // advertise to central, check for connections within timeout. Timeout measured in seconds

  if ((millis() - timeoutStart) < (timeoutDuration * 1000)) {
    // Timeout has not been reached. Attempt to connect to central
    central = getCentral();
    if (central.connected()) {
      connected = true;
      inPairingMode = false;
      Serial.println("Central Connected!");
    }

    else {
      // Serial.println("No Central Detected");
    }
  }
  
  else {
    // Reached timeout
    reachedTimeout = true;
    BLE.stopAdvertise();
    Serial.println("Timeout reached, connection failed");
    inPairingMode = false;
  }

}

// ---------------- Config Mode Methods and Callbacks --------------- //
void BLEManager::enterConfigMode(int timeout) {
  // Set Flag that we're accepting new config values
  BLE.setAdvertisedService(configInfoService);
  BLE.advertise();
  enterPairingMode(timeout);
}

void BLEManager::exitConfigMode() {
  // Set Flag that we're accepting new config values
  acceptNewConfig = false;
  central = getCentral();
  if (central.connected()){
    central.disconnect();
  }
}

void BLEManager::onDateTimeCharWritten(BLEDevice central, BLECharacteristic characteristic){
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  dateTimeStr = bytesToString(data, length);
  dateTimeRefrenceMillis = millis();
  Serial.println("New DateTime Characteristic Recieved: " + dateTimeStr);
  fileName = updateFileName();
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);
}
void BLEManager::onPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic){
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  personName = bytesToString(data, length); 
  Serial.println("New personName Characteristic Recieved: " + personName);
  fileName = dateTimeStr + "-" + personName + "-" + activityType;
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);

}

void BLEManager::onActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  activityType = bytesToString(data, length);
  Serial.println("New activityType Characteristic Recieved: " + activityType);
  fileName = dateTimeStr + "-" + personName + "-" + activityType;
  Serial.println("New File Name: " + fileName);
  fileNameConfigChar.writeValue(fileName);
}

BLEDevice BLEManager::getCentral() {
  return BLE.central();
}


// ------------- IMU Tx and Record Mode Methods and Callbacks ----------- //
bool BLEManager::enterIMUTxRecordMode(int timeout) {
  // Check to see if there's a valid file name
  // Initiate a connection with client
  // Set Flag that we're accepting new config values
  BLE.setAdvertisedService(IMUServiceUuid);
  BLE.advertise();
  enterPairingMode(timeout);

  // Do we need to initiate the data recording here? Where do we do that?
}

void BLEManager::exitIMUTxRecordMode() {
  acceptIMUTxRequest = false;
  imuTxActive = false;
  dataRecorder.stopDataRecording(dataFileName.c_str());
  central = getCentral();
  if (central.connected()){
    central.disconnect();
  }
}

void BLEManager::onIMUTxRequest(BLEDevice central, BLECharacteristic characteristic) {
    int length = characteristic.valueLength();
    byte data[length];
    characteristic.readValue(data, length);
    String imuRequest = bytesToString(data, length);
    
    Serial.println("Request Recieved: " + imuRequest);
    // If we've recieved a start command, switch the flag to true
    if (imuRequest.equals("START")) {
      imuTxActive = true;
      String dataFileName = updateFileName();
      dataRecorder.startDataRecording(dataFileName.c_str());
    }

    else if (imuRequest.equals("END")) {
      acceptIMUTxRequest = false;
      imuTxActive = false;
      exitIMUTxRecordMode();
    }
}

bool BLEManager::imuRecordandTx() {
  // Function to read, record and transmit line of imu data to SD card and characteristic
  // Returns a true or false. This communicates a change in mode to the main program if
  // we exit this mode from a command from the client rather than a button press
  if (!imuTxActive) {
    central = getCentral();
    if (central.connected()) {
      return true;
    }

    else {
      return false;
    }
  }

  else {
    char* dataLine = dataRecorder.readIMU();
    imuDataChar.setValue(dataLine);
    return true;
  }
}

//---------------- File Tx Methods and Callbacks ------------------//

/* Skeleton functions:
Connection Procedure
  advertise service to client
  connect to client
  client requests for files to be transferred
file transfer
  prephrial searches SD card for whitelist file
  Create list of file names to transfer
  loop - FileTxActive = True
    first iteration: open file, tx file name
      wait for central to confirm readiness to recieve file 
    if fileDataTxActive
      transfer file contents
      check if end of file
        set fileDatatxActive to false
        set endFileData to True
    if endFileData
      delete file
      remove from whitelist file
      Notify end of file
  


*/
void BLEManager::onFileTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  // Handle event for central writing to the dateTime characteristic
  int length = characteristic.valueLength();
  byte data[length];
  characteristic.readValue(data, length);
  String fileTxRequest = bytesToString(data, length);
  
  Serial.println("Request Recieved: " + fileTxRequest);
  
  if (fileTxRequest.equals("SEND_FILES")) {
    // Open and load contents of the whitelist file
    File32 whiteListFile;
    whiteListFile.open(dataRecorder.whiteListFilePath, O_READ);
    // Create a list of file names from the whitelist file (lines separated by "\n"?)
    // Save list to varaible
    while (whiteListFile.available()) {
      String line = whiteListFile.readStringUntil('\n');
      line.trim();
      // Serial.println(line);
      if (line.length() > 0) {
          whiteListFileNames.push_back(line);
      }
    }

    if (whiteListFileNames.empty()) {
      Serial.println("No files available to transfer!");
      fileTxRequestChar.writeValue("ERROR!");
      return;
    }
    
    Serial.println("List of File Names in WhiteList:");
    for (String l : whiteListFileNames) {
        Serial.println(l);
    }

    Serial.println();
    Serial.println("Ready to send files!");

    fileTxRequestChar.writeValue("READY");
    fileTxActive = true;
  }  
  
  // If "Start" is written to the characteristic, then the client is ready to recieve data.
  // Set txFileFlag to true so a new packet is sent every loop. 
  else if (fileTxRequest.equals("START")) {
    // Set flag to true
    fileDataTxActive = true;
  }

  else if (fileTxRequest.equals("MORE_FILES?")) {
    // If the index value is less than the length of file lists to transmit
    // If the list has been exhausted, exit transmit mode. Confirm file deletion
    if (txFileListIndex < (whiteListFileNames.size() - 1)) {
      Serial.print("Notifying Central of more files!");
      Serial.print("Number of files: ");
      Serial.println(whiteListFileNames.size());
      Serial.print("Value of txFileListIndex: ");
      Serial.println(txFileListIndex);

      Serial.print("Number of files left: ");
      Serial.println(whiteListFileNames.size() - txFileListIndex);
      fileTxRequestChar.writeValue("MORE_FILES");
      fileTxActive = true;
      }

    else {
      // Code to run completion of the WhiteList transfer
      // Clear contents of the whitelist file. Save this for later, implement when
      // comfortable deleting data off the device as well.
      // dataRecorder.clearWhiteList();
      Serial.println("No more files!");
      fileTxRequestChar.writeValue("DONE");
      fileTxActive = false;
      txFileListIndex = 0;
    }
    fileSetup = false;
  }
}
bool BLEManager::enterFileTxMode(int timeout) {
  BLE.setAdvertisedService(fileTxService);
  BLE.advertise();
  return true;
}

void BLEManager::exitFileTxMode() {
  return;
}

bool BLEManager::txFileData() {
  if (fileTxActive) {
    /*
    Copy paste from feature demonstration. Awaiting implementation in integrated environment

    first iteration: open file, tx file name
      wait for central to confirm readiness to recieve file 
    if fileDataTxActive
      transfer file contents
      check if end of file
        set fileDatatxActive to false
        set endFileData to True
    if endFileData
      delete file
      remove from whitelist file
      Notify end of file

    */
    // If on first iteration of fileTxActive loop, fileTxDataFlag will be false.
    // open the specified file
    if (!fileDataTxActive && !fileSetup) {
      String txFileName = whiteListFileNames[txFileListIndex];
      String txFilePath = "accelDir/" + txFileName;
      Serial.print("Attempting to open file: ");
      Serial.println(txFilePath);
            // Check to see if file exists
      if (!dataRecorder.sd.exists(txFilePath)) {
        Serial.println("ERROR: File not recognized!");
        fileTxActive = false;
        return false;
      }

      txFile.open(txFilePath.c_str(), O_READ | O_BINARY);
      fileNameTxChar.writeValue(txFileName);
      
      // Create a time counter for funsies
      txStartTime = millis();
      
      Serial.println("File opened. Transmitting...");
      fileSetup = true;
      // fileDataTxActive = true;
    }
    
    else if (!fileDataTxActive && fileSetup) {
    
    }
    
    // Parse through the datafile, chunking data into payloads and transmitting
    // each payload sequentially
    else if (txFile.available()) {
      char txBuffer[fileTxBufferSize]; // Create a buffer to fill with data
      int bytesRead = txFile.read(txBuffer, fileTxBufferSize);
      // txBuffer[bytesRead] = '\0'; // Null-terminate

      // Serial.print("Bytes read from file: ");
      // Serial.println(bytesRead);  // Should be > 0
      // Serial.print("Buffer contents: ");
      // Serial.println(buffer);     // Should show file contents
      // Debug print contents in hex
      Serial.print("Buffer Contents: ");
      Serial.println(txBuffer);
      // Write the full buffer to the characteristic
      char* testPacket = "Test Packet";
      bool success = fileTxDataChar.writeValue((uint*)txBuffer, bytesRead);

      // Test line with hardcoded value   
      // const char* testData = "HELLO FROM PERIPHERAL";
      // bool success = fileTxDataChar.writeValue((const uint*)testData, strlen(testData));
      delay(30);

      /*
      if (!success) {
        Serial.println("ERROR: Failed to write to characteristic!");
      } 
      else {
        Serial.println("Write successful");
      }
      */
      // Serial.print("Packet Written to Central: ");
      // Serial.println(buffer);
      // delay(500); // Small delay to prevent BLE stack overflow
    }

    else {
      int txElapsedTime = (millis() - txStartTime) / 1000;
      Serial.print("File transmission completed in: ");
      Serial.println(txElapsedTime);
      fileDataTxActive = false;
      fileTxActive = false;

      txFile.close();
      fileTxCompleteChar.writeValue("TRANSFER_COMPLETE");
      txFileListIndex++;


        // Should we have a query to the client to confirm file deletion? Probably
    }
    return true;
  }

  else {
    return true;
  }
  // if the flag is 
}
/*
//------------------ BLE Connection Event Callbacks ---------------- //
void onConnect(BLEDevice central) {
  // Callback for connection device to central
  // connected = true;
}

void onDisconnect(BLEDevice central) {
  // Callback for disconnection device
  // connected = false;
}
*/