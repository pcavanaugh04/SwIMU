//========================================================//
/* Header File for Data Recorder library for SwIMU project
This library combines the functionality of the onboard IMU sensore
on an XIAO nrf52840 BLE Sense board with an SD breakout board to
measure and record data to file

Contains:
  Classes:
    DataRecorder
  
  Functions

*/
//==========================================================//

#ifndef DATARECORDER_H
#define DATARECORDER_H

#include <Arduino.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
// Global Variables
const unsigned long microsOverflowValue = 4294967295;

// Functions

// Classes
class DataRecorder {
  // Private Variables and Functions
  private:
    // Accelerometer Variables
    unsigned long imuStartMillis;       // Timestamp for start of data recording
    unsigned long numSamples = 0;         // Sample counter during data recording
    float imuTime;                      // time variable for recording data
    // const float dataRateHz;  // Use to program the dataRate of the accelerometer
    // const float dataPerioduS = 1 / dataRateHz * 1000000;  // Config data read event
    unsigned long dataReadTime;           // timestamp for last data read event
    LSM6DS3 imuSensor;                        // Object for onbarod IMU sensor
    const char rootDir[12] = "accelDir/";  // Default data directory onboard SD card
    const char whiteListFileName[15] = "whiteList.txt";

    // char whiteListPath[25] = "accelDir/whitelist.txt";
    bool recording = false;
    // SD Card file variables
    // int currentCount;
    File32 imuDataFile;                   // data file object to be used for writing during record mode
    int fileNameLength = 150;
    // int chipSelect{};                     // Chip select pin to be input by the constructor
    void updateWhiteList(const char* fileName);               // Update whitelist of files for transmitting
  
  // Public variables and functions
  public:
    SdFat32 sd;
    DataRecorder(); // Constructor
    void displayDirectory(const char* dirName="/", int numTabs=0);  // Print out the contents of the onbaord SD card
    char* readIMU();                      // Read Values from the onboard IMU
    void startDataRecording(const char* fileName);  // Start Data recording with a specified fileName
    void stopDataRecording(const char* fileName);             // Stop data recording and close file
    void clearWhiteList();                // Clear Whitelist of files after transmitting
    void initDevices(int chipSelect);
    char whiteListFilePath[100]; // Make sure this is large enough
    int getFileNameLength();

};

#endif 