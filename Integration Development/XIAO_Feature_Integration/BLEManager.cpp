//========================================================//
/* Header File for BLE Manager for SwIMU project
This library manages all the BLE functions of the board, namely
configuring with the client, realtime data transmission, and 
file transmission

Contains:
  Classes:
    BLEManager
  
  Functions

*/
//==========================================================//


// Module Goals:
// Get definitions for all prototypes in .h file
// Enable File Config with Client
// Enable live Tx and data record
// Enable file search and transfer


#include "BLEManager.h"

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
      fileTxRequestChar(fileTxRequestCharUuid, BLEWrite | BLERead, 10),
      fileTxDataChar(fileTxDataCharUuid, BLENotify, fileTxBufferSize, false),
      fileTxCompleteChar(fileTxCompleteCharUuid, BLENotify, 30),
      fileNameResponseChar(fileNameResponseCharUuid, BLERead, 60) {

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
  fileTxService.addCharacteristic(fileNameResponseChar);
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

String BLEManager::getFileName() {
  return (getDateTimeStr() + "-" + personName + "-" + activityType + ".csv");
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
    return "ERROR!";
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
      Serial.println("No Central Detected");
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
  fileName = getFileName();
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
  String dataFileName = getFileName();
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
      String dataFileName = getFileName();
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
      Serial.println(line);
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
    fileTxActive = true;
    fileTxRequestChar.writeValue("READY");
  }  
  
  // If "Start" is written to the characteristic, then the client is ready to recieve data.
  // Set txFileFlag to true so a new packet is sent every loop. 
  else if (fileTxRequest.equals("START")) {
    // Set flag to true
    fileTxActive = true;
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
    /* Copy paste from feature demonstration. Awaiting implementation in integrated environment

    // Open the specified file
    String filePath = "accelDir/" + dataFileName;
    
    Serial.print("Attempting to open file: ");
    Serial.println(filePath);
          // Check to see if file exists
    if (!SD.exists(filePath)){
      Serial.println("ERROR: FIle not recognized!");
    }
    File dataFile = SD.open(filePath.c_str(), FILE_READ);
    Serial.println(dataFile.name());
    // Create a time counter for funsies
    int txStartTime = millis();
    Serial.println("File opened. Transmitting...");
    // Parse through the datafile, chunking data into payloads and transmitting
    // each payload sequentially
    while (dataFile.available()) {
      char buffer[fileTxBufferSize]; // Create a buffer to fill with data
      int bytesRead = dataFile.read(buffer, fileTxBufferSize);
      // Serial.print(buffer);
      // Write the full buffer to the characteristic
      fileTransferDataChar.writeValue((uint*)buffer, bytesRead);
      delay(30); // Small delay to prevent BLE stack overflow
    }


    int txElapsedTime = millis() - txStartTime;
    // Send a notification when the end is reached
    fileTransferCompleteChar.writeValue("TRANSFER_COMPLETE");
    Serial.print("File transmission completed in:");
    Serial.print(txElapsedTime);

    // Close the file
    dataFile.close();
  */
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