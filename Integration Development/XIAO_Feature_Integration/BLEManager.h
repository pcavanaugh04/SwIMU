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

#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <string.h>
#include <map>
#include "DataRecorder.h"

// ----------------------------- BLE Configs ---------------------------- //

// Init Objects

// Functions
String bytesToString(byte* data, const int length);

// Static BLE Callbacks
static void staticOnIMUTxRequest(BLEDevice central, BLECharacteristic characteristic);
static void staticOnDateTimeCharWritten(BLEDevice central, BLECharacteristic characteristic);
static void staticOnPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic);
static void staticOnActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic);
static void staticOnFileTxRequest(BLEDevice central, BLECharacteristic characteristic);
static void staticOnConnect(BLEDevice central);
static void staticOnDisconnect(BLEDevice central);

// Classes
class BLEManager {
  // Private Variables and Functions
  private:

  // File Config Service and Characteristics
    const char* configInfoServiceUuid = "550e8400-e29b-41d4-a716-446655440000";
    const char* dateTimeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440001";
    const char* personNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440002";
    const char* activityTypeConfigCharUuid = "550e8401-e29b-41d4-a716-446655440003";
    const char* fileNameConfigCharUuid = "550e8401-e29b-41d4-a716-446655440004";

    // IMU Data Stream Service and Characteristics
    const char* IMUServiceUuid = "550e8402-e29b-41d4-a716-446655440000";
    const char* IMURequestCharUuid = "550e8403-e29b-41d4-a716-446655440001";
    const char* IMUDataCharUuid = "550e8403-e29b-41d4-a716-446655440002";

    // File Tx Service and Characteristics
    const char* fileTxServiceUuid = "550e8404-e29b-41d4-a716-446655440000";
    const char* fileTxRequestCharUuid = "550e8405-e29b-41d4-a716-446655440001";
    const char* fileTxDataCharUuid = "550e8405-e29b-41d4-a716-446655440002";
    const char* fileTxCompleteCharUuid = "550e8405-e29b-41d4-a716-446655440003";
    const char* fileNameResponseCharUuid = "550e8405-e29b-41d4-a716-446655440004";
    // Init Characteristics and Services:
        // BLE Configuration Service and Characteristics
    BLEService configInfoService;
    BLEStringCharacteristic dateTimeConfigChar;
    BLEStringCharacteristic personNameConfigChar;
    BLEStringCharacteristic activityTypeConfigChar;
    BLEStringCharacteristic fileNameConfigChar;

    // BLE IMU Data Transmission Service and Characteristics
    BLEService imuTxService;
    BLEStringCharacteristic imuRequestChar;
    BLECharacteristic imuDataChar;

    // BLE File Transmission Service and Characteristics
    BLEService fileTxService;
    BLEStringCharacteristic fileTxRequestChar;
    BLECharacteristic fileTxDataChar;
    BLEStringCharacteristic fileTxCompleteChar;
    BLEStringCharacteristic fileNameResponseChar;
        
    // Other functions
    void findFilestoTx();
    void txFileData();
    // Variables
    DataRecorder& dataRecorder;
    BLEDevice central;
    BLEDevice getCentral();
    const int fileTxBufferSize = 244; // Oddly enough this seems to be the bandwidth of the string Char
    unsigned long dateTimeRefrenceMillis; // refrence value for incrementing local timestamp
    const char* deviceName = "SwIMU";
    bool acceptNewConfig;
    bool acceptIMUTxRequest;
    bool imuTxActive;
    String dateTimeStr;
    String personName;
    String activityType;
    String fileName;

  // Public variables and functions
  public:
    BLEManager(DataRecorder& dataRecorder); // Constructor
    bool enterConfigMode();
    void exitConfigMode();
    bool enterIMUTxRecordMode();
    void exitIMUTxRecordMode();
    bool imuRecordandTx();
    bool enterFileTxMode();
    void exitFileTxMode();
    // BLE Callbacks;
    void onIMUTxRequest(BLEDevice central, BLECharacteristic characteristic);
    void onDateTimeCharWritten(BLEDevice central, BLECharacteristic characteristic);
    void onPersonNameCharWritten(BLEDevice central, BLECharacteristic characteristic);
    void onActivityTypeCharWritten(BLEDevice central, BLECharacteristic characteristic);
    void onFileTxRequest(BLEDevice central, BLECharacteristic characteristic);
    void onConnect(BLEDevice central);
    void onDisconnect(BLEDevice central);
    // bool IMUTxMode(int timeout);
    // bool FileTxMode(int timeout);  
    bool connected = false;                // Read Values from the onboard IMU
    // void updateDateTimeStr();
    void startBLE();
    String getDateTimeStr();
    String getFileName();
    void poll();

};

#endif 