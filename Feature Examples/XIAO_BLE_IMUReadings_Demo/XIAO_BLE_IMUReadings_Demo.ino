
#include <SPI.h>
#include <SD.h>
#include <ArduinoBLE.h>
#include <LSM6DS3.h>
#include <string.h>

// SD Card Setup
const int chipSelect = 2;
int currentCount;

// BLE Setup
const char * deviceServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
// Ensure First block of characteristic UUID is incremented +1 of Service UUID
const char * deviceServiceRequestCharacteristicUuid = "550e8401-e29b-41d4-a716-446655440001";
const char * deviceServiceResponseCharacteristicUuid = "550e8401-e29b-41d4-a716-446655440002";

// Accelerometer Variables
unsigned long accelStartMillis;
unsigned long numSamples = 0;
float accelTime;
//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A



BLEService imuService(deviceServiceUuid);
BLEStringCharacteristic imuRequestCharacteristic(deviceServiceRequestCharacteristicUuid, BLEWrite, 4);
BLEStringCharacteristic imuResponseCharacteristic(deviceServiceResponseCharacteristicUuid, BLERead | BLENotify, 512);

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
  /*
  File file = SD.open("/test2.txt");
  if (file) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Serial.println(line);
    }
    file.close();
  }
  else {
    Serial.println("Error opening file!");
  }
  */
  /*
  // Create Accelerometer Data Folder
  if(SD.mkdir("accelDir")) {
    Serial.println("Create directory success!");
  }
  */
  // SD.remove("counter.txt");
  // SD.rmdir("accelDir");
  
//  File newCounterFile = SD.open("accelDir/counter.txt", FILE_WRITE);
//  newCounterFile.print("0");
//  newCounterFile.close();
  
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

  // broadcast service to central devices for connections
  imuService.addCharacteristic(imuRequestCharacteristic);
  imuService.addCharacteristic(imuResponseCharacteristic);
  BLE.addService(imuService);
  BLE.setAdvertisedService(imuService);

  imuResponseCharacteristic.writeValue("90");
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
    // Delay in connection to allow central time to config services
    delay(1);
  }
  // String newValues = readAccel();
  // Serial.print("New Readings from IMU: ");
  // Serial.println(newValues);
  while (central.connected()) {
    // We might need some way of notifying the device that everything is setup to go.
    // This prevents the characteristic from updating while its being configured from central
    // Serial.println("Central Connected!");
    // If we want to respond to a query, use this if structure a reading from the sensor
    // if (imuResponseCharacteristic.written()) {}
    // String newValues = readAccel();
    String newValues = readAccel();
    Serial.print("Value from readAccel: ");
    Serial.println(newValues);
    imuResponseCharacteristic.setValue(newValues);

    delay(10);

  }
}
