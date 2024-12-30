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
#include <ArduinoBLE.h>

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
    char rootDir[10] = "accelDir/";  // Default data directory onboard SD card
    char whiteListPath[25] = "accelDir/whitelist.txt";
    bool recording = false;
    // SD Card file variables
    // int currentCount;
    File imuDataFile;                   // data file object to be used for writing during record mode
    int chipSelect{};                     // Chip select pin to be input by the constructor

    void updateWhiteList();               // Update whitelist of files for transmitting
  
  // Public variables and functions
  public:
    DataRecorder(int chipSelect); // Constructor
    DataRecorder() = default;             //
    void displayDirectory(const char* dirName, int numTabs);  // Print out the contents of the onbaord SD card
    char* readIMU();                      // Read Values from the onboard IMU
    void startDataRecording(const char* fileName);  // Start Data recording with a specified fileName
    void stopDataRecording();             // Stop data recording and close file
    void clearWhiteList();                // Clear Whitelist of files after transmitting
};

#endif 