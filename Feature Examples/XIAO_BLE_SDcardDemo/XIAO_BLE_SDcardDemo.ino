
#include <SPI.h>
#include <SD.h>

const int chipSelect = 2;

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);
  digitalWrite(chipSelect, HIGH);
  Serial.println("Initializing SD Card...");

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
  
  
  File counterFileContents = SD.open("accelDir/counter.txt", FILE_READ);
  if (counterFileContents) {
    int currentCount = counterFileContents.parseInt();
    counterFileContents.close();
    currentCount++;
    char fileName[40];
    sprintf(fileName, "DATA_%d", currentCount);
    Serial.print("Current file count number: ");
    Serial.println(currentCount);
    SD.remove("accelDir/counter.txt");
    File newCounterFile = SD.open("accelDir/counter.txt", FILE_WRITE);
    newCounterFile.print(currentCount, DEC);
    newCounterFile.close();
  }
  
  else Serial.println("Error opening Counter File!");
  
  /*
  What else do we need to do?

  have some way of indexing file names. Counter variable living in SD card "counter.txt"

  */
}

void loop() {
  // put your main code here, to run repeatedly:

}
