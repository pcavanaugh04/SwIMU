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
      fileTxRequestChar(fileTxRequestCharUuid, BLEWrite, 10),
      fileTxDataChar(fileTxDataCharUuid, BLENotify, fileTxBufferSize, false),
      fileTxCompleteChar(fileTxCompleteCharUuid, BLENotify, 30),
      fileNameResponseChar(fileNameResponseCharUuid, BLERead, 60) {
  // Initialize Data recorder
  dataRecorder = dataRecorder;

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
  BLE.advertise();
  Serial.println("BLE device ready to connect");

}


// ------------------ Getters and Setters -------------------- //

String BLEManager::getFileName() {
  return (getDateTimeStr() + "-" + personName + "-" + activityType;
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
    Serial.println("Error: Invalid datetime format.");
    return "ERROR!";
  }

  // Calculate new values for hours, minutes and seconds based on the number of new seconds
  unsigned long newSeconds = (millis() - dateTimeRefrenceMillis) / 1000;
  int newTotalSeconds = (hours * 3600) + (minutes * 60) + seconds + newSeconds;
  hours = newTotalSeconds / 3600;
  minutes = newTotalSeconds % 3600 / 60;
  seconds = newTotalSeconds % 3600 % 60;
  char buffer[25];
  sprintf(buffer, "%d_%d_%d_%d_%d_%d", year, month, day, hours, minutes, seconds);
  dateTimeStr = String(buffer);
  return dateTimeStr;
}

// ---------------- Config Mode Methods and Callbacks --------------- //
bool BLEManager::enterConfigMode() {
  // Set Flag that we're accepting new config values
  central = getCentral();
  if (central.connected()) {
    acceptNewConfig = true;
    return true;
  }

  else {
    Serial.println("Central not connected. Cannot configure!");
    return false;
  }
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
  if (acceptNewConfig) {
    int length = characteristic.valueLength();
    byte data[length];
    characteristic.readValue(data, length);
    dateTimeStr = bytesToString(data, length);
    dateTimeRefrenceMillis = millis();
    Serial.println("New DateTime Characteristic Recieved: " + dateTimeStr);
    fileName = dateTimeStr + "-" + personName + "-" + activityType;
    Serial.println("New File Name: " + fileName);
    fileNameConfigChar.writeValue(fileName);
  }
  else {
    Serial.println("Not accepting new config messages!");
  }
}
void BLEManager::onPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic){
  // Handle event for central writing to the dateTime characteristic
  if (acceptNewConfig) {
    int length = characteristic.valueLength();
    byte data[length];
    characteristic.readValue(data, length);
    personName = bytesToString(data, length); 
    Serial.println("New personName Characteristic Recieved: " + personName);
    fileName = dateTimeStr + "-" + personName + "-" + activityType;
    Serial.println("New File Name: " + fileName);
    fileNameConfigChar.writeValue(fileName);
  }
  else {
    Serial.println("Not accepting new config messages!");
  }
}

void BLEManager::onActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic) {

  if (acceptNewConfig) {
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
  else {
    Serial.println("Not accepting new config messages!");
  }
}

BLEDevice BLEManager::getCentral() {
  return BLE.central();
}


// ------------- IMU Tx and Record Mode Methods and Callbacks ----------- //
bool BLEManager::enterIMUTxRecordMode() {
  // Check to see if there's a valid file name
  // Initiate a connection with client
  central = getCentral();
  if (central.connected()) {
    acceptIMUTxRequest = true;
    return true;
  }

  else {
    Serial.println("Central not connected. Cannot Tx IMU Readings!");
    return false;
  }
}
void BLEManager::exitIMUTxRecordMode() {
  acceptIMUTxRequest = false;
  imuTxActive = false;
  dataRecorder.stopDataRecording();
  central = getCentral();
  if (central.connected()){
    central.disconnect();
  }
}

void BLEManager::onIMUTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  if (acceptIMUTxRequest) {
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

    else if (imuRequest.equals("STOP")) {
      imuTxActive = false;
      exitIMUTxRecordMode();
    }
  }

  else {
    Serial.println("Central not connected, cannot transmit IMU data!");
  }
}

bool BLEManager::imuRecordandTx() {
  // Function to read, record and transmit line of imu data to SD card and characteristic
  // Returns a true or false. This communicates a change in mode to the main program if
  // we exit this mode from a command from the client rather than a button press
  if (imuTxActive) {
    char* dataLine = dataRecorder.readIMU();
    imuDataChar.setValue(dataLine);
    return true;
  }

  else {
    return false;
  }
}

//---------------- File Tx Methods and Callbacks ------------------//
void BLEManager::onFileTxRequest(BLEDevice central, BLECharacteristic characteristic) {
  return;
}

bool BLEManager::enterFileTxMode() {
  return true;
}

void BLEManager::exitFileTxMode() {
  return;
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