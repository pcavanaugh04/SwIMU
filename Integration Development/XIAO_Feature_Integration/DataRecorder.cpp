

//========================================================//
/* Header File for Data Recorder library for SwIMU project
Contains:
  Classes

  Functions

*/
//==========================================================//

#include "DataRecorder.h"

// Class Constructor
DataRecorder::DataRecorder() {
  // Serial.println(chipSelect);
  // chipSelect = chipSelect;
  // Initialize IMU Sensor
  imuSensor = LSM6DS3(I2C_MODE, 0x6A);
}

void DataRecorder::initDevices(int chipSelect) {
  // This needs to be called in the main .setup() function!
  // Or else the program will not upload
  if (imuSensor.begin() != 0) {
    Serial.println("IMU initialization failed!");
  }
  else {
    Serial.println("IMU initialized successfully.");
  }

  // Initialize SD Card
  Serial.print("Initializing on pin: ");
  Serial.println(chipSelect);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");
} 

void DataRecorder::displayDirectory(const char* dirName, int numTabs) {
  File dir = SD.open(dirName);
  while (true) {
        File entry = dir.openNextFile();
        if (!entry) break; // No more files

        for (int i = 0; i < numTabs; i++) {
            Serial.print("\t");
        }

        Serial.print(entry.name());
        if (entry.isDirectory()) {
            Serial.println("/");
            this->displayDirectory(entry.name(), numTabs + 1); // Recursive call for subdirectories
        } else {
            Serial.print("\t\t");
            Serial.println(entry.size());
        }
        entry.close();
    }
}

// Function to read IMU Data
char* DataRecorder::readIMU() {
  static char imuBuffer[40];

  imuTime = (float)(millis() - imuStartMillis) / 1000;
  float accelX = imuSensor.readFloatAccelX();
  float accelY = imuSensor.readFloatAccelY();
  float accelZ = imuSensor.readFloatAccelZ();
  float gyroX = imuSensor.readFloatGyroX();
  float gyroY = imuSensor.readFloatGyroY();
  float gyroZ = imuSensor.readFloatGyroZ();
  numSamples++;
  // Format in a string that will follow a CSV format
  char accelBuffer[40];
  sprintf(imuBuffer, "%.4f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", imuTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
  // Print data line to serial monitor
  if (recording) {
    imuDataFile.println(imuBuffer);
  }
  return accelBuffer;
}

void DataRecorder::startDataRecording(const char* fileName) {
  imuDataFile = SD.open(fileName, FILE_WRITE);
  if (!imuDataFile) {
    Serial.println("Failed to open file");
  }

  Serial.println("Data recording started");
  imuStartMillis = millis();
  recording = true;
}

void DataRecorder::stopDataRecording() {
  if (imuDataFile) {
    updateWhiteList();
    imuDataFile.close();
    Serial.println("Data recording stopped and file closed");
  }
  else {
    Serial.println("No available file to close!");
  }
  float calcHz = numSamples / imuTime;
  Serial.print("Calculated Data Rate: ");
  Serial.println(calcHz);
  recording = false;
  numSamples = 0;
}

void DataRecorder::updateWhiteList() {
  // This function updates the whitelist file that records files that haven't yet been transmitted
  File whiteList = SD.open(whiteListPath, FILE_WRITE);
  whiteList.println(imuDataFile.name());
  whiteList.close();
}

void DataRecorder::clearWhiteList() {
  // Clear contents of the whitelist file. To be called after all files have been sent over BLE
  File whiteList = SD.open(whiteListPath, O_TRUNC);
  whiteList.close();
}