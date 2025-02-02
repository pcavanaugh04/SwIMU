//========================================================//
/* C++ File for Data Recorder library for SwIMU project
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
  if (!sd.begin(chipSelect, SD_SCK_MHZ(25))) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");
} 

void DataRecorder::displayDirectory(const char* dirName, int numTabs) {
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
        char buf[60];
        entry.getName(buf, sizeof(buf));
        Serial.print(buf);
        if (entry.isDir()) {
            Serial.println("/");

            this->displayDirectory(buf, numTabs + 1); // Recursive call for subdirectories
        } else {
            Serial.print("\t\t");
            Serial.println(entry.fileSize());
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
  sprintf(imuBuffer, "%.4f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", imuTime, accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
  // Print data line to serial monitor
  if (recording) {
    imuDataFile.println(imuBuffer);
  }
  return imuBuffer;
}

void DataRecorder::startDataRecording(const char* fileName) {
  char filePath[50];
  snprintf(filePath, sizeof(filePath), "%s%s", rootDir, fileName);

  if (!imuDataFile.open(filePath, O_WRITE | O_CREAT | O_TRUNC)) {
    Serial.print("Failed to open file: ");
    Serial.println(filePath);
    return;
  }

  Serial.print("Data recording started in: ");
  Serial.println(filePath);

  imuStartMillis = millis();
  numSamples = 0;
  recording = true;
}

void DataRecorder::stopDataRecording() {
  if (imuDataFile.isOpen()) {
    if (imuDataFile.close()){
      Serial.println("Data recording stopped and file closed");

    };
    delay(500);
    updateWhiteList();
  }
  else {
    Serial.println("No available file to close!");
  }
  float calcHz = numSamples / imuTime;
  Serial.print("Calculated Data Rate: ");
  Serial.println(calcHz);
  recording = false;
}

void DataRecorder::updateWhiteList() {
  // This function updates the whitelist file that records files that haven't yet been transmitted
  char newFilePath[50];
  char* fileName = "send.txt";
  snprintf(newFilePath, sizeof(newFilePath), "%s%s", rootDir, fileName);

  File32 whiteList;
  if (!whiteList.open(newFilePath, O_WRITE | O_CREAT | O_APPEND)) {
    Serial.println("Error in opening whitelist file!");
    return;
  }
  char buf[60];
  imuDataFile.getName(buf, sizeof(buf));
  whiteList.println(buf);
  whiteList.close();
  delay(500);
  Serial.println("Whitelist file updated");
}

void DataRecorder::clearWhiteList() {
  char filePath[50];
  snprintf(filePath, sizeof(filePath), "%s%s", rootDir, "send.txt");

  File32 whiteList;
  if (whiteList.open(filePath, O_WRITE | O_TRUNC)) {
    whiteList.close();
    Serial.println("Whitelist cleared.");
  } else {
    Serial.println("Failed to clear whitelist.");
  }
}