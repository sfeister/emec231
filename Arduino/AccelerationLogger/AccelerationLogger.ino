/* Acceleration Logger
    Creates data logging CSV file and appends acceleration data periodically

    Built for Arduino Uno

    SD Card Arduino Shield:
      SD Card Shield 3.0 - note the "5V" setting is correct on the switch
      https://wiki.iteadstudio.com/Stackable_SD_Card_shield_V3.0

    Accelerometer:
      Model: GY-521
      How to Get Started Using: https://projecthub.arduino.cc/Nicholas_N/how-to-use-the-accelerometer-gyroscope-gy-521-647e65
      Wiring:
        "VCC" on GY-521 -> "5 V" on Arduino 
        "GND" on GY-521 -> "GND" on Arduino
        "SCL" on GY-521 -> "A5" on Arduino
        "SDA" on GY-521 -> "A4" on Arduino

    Required Libraries: Make sure to install the Chrono library from the Arduino Libraries repository.
      The Chrono library is for setting timers : http://github.com/SofaPirate/Chrono . 
      We will use the Chrono BasicMetro object: https://github.com/SofaPirate/Chrono/blob/master/examples/basicMetro/basicMetro.ino
            
    Written by Scott Feister on October 25, 2023.
    Includes code sections copied or modified from various example libraries:
      1. SD card Datalogger.ino example (from the library examples folder)
      2. GY-521 examples at https://projecthub.arduino.cc/Nicholas_N/how-to-use-the-accelerometer-gyroscope-gy-521-647e65
      3. Chrono basicMetro.ino example at https://github.com/SofaPirate/Chrono/blob/master/examples/basicMetro/basicMetro.ino
*/

#include <SPI.h> // For writing to the SD card
#include <SD.h> // For writing to the SD card
#include<Wire.h> // For accessing acceleration data from the GY-521 Accelerometer
#include <Chrono.h> // For setting timers to record data at a measured rate

// Initialize SD-related variables
const int chipSelect = 4;
File dataFile;

// Initialize GY-521-related variablles
const int MPU=0x68; // GY-521
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ; // GY-521

// Initialize timer-related variables
Chrono myChrono; 
unsigned long deltat;

void  setup(){
  /** Initialize serial communication over USB (optional for debugging) **/
  Serial.begin(9600);

  /** Setup GY-521 wire data communication **/
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  
  Wire.write(0);    
  Wire.endTransmission(true);

  /** Setup SD card data communication **/
  Serial.println("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    Serial.println("Fix, then reset Arduino to restart datalogging");
    // don't do anything more:
    while (1);
  }
  Serial.println("SD card initialized.");

  /** Create or overwrite file 'datalog.txt' on SD card **/
  // delete the file if it already exists on the SD card, to begin with an empty datalog
  if (SD.exists("datalog.txt")) {
    SD.remove("datalog.txt");
  }

  // open the file. note that only one file can be open at a time,
  // so if you were try to access a second file you'd need to close this one.
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    // Assemble the file header as a comma-separated string:
    String headerString = "";
    headerString += "deltat,";
    headerString += "AcX,";
    headerString += "AcY,";
    headerString += "AcZ,";
    headerString += "GyX,";
    headerString += "GyY,";
    headerString += "GyZ,";
    dataFile.println(headerString);

    dataFile.close();
  }
  else {
    Serial.println("error opening datalog.txt");
  }

  myChrono.restart();  // restart the timer
}

void loop(){
  if (myChrono.hasPassed(20)) { // check the timer; has 250 ms elapsed?
    deltat = myChrono.elapsed(); // note the number of milliseconds elapsed since last record (should be close to 250 ms)
    myChrono.restart();  // restart the timer
    
    /** Grab instantaneous acceleration data from GY-521 **/
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);  
    Wire.endTransmission(false);
    Wire.requestFrom(MPU,12,true);  
    AcX=Wire.read()<<8|Wire.read();    
    AcY=Wire.read()<<8|Wire.read();  
    AcZ=Wire.read()<<8|Wire.read();  
    GyX=Wire.read()<<8|Wire.read();  
    GyY=Wire.read()<<8|Wire.read();  
    GyZ=Wire.read()<<8|Wire.read();  

    /** Assemble the data into a comma-separated string: **/
    String dataString = "";
    dataString += String(deltat) + ",";
    dataString += String(AcX) + ",";
    dataString += String(AcY) + ",";
    dataString += String(AcZ) + ",";
    dataString += String(GyX) + ",";
    dataString += String(GyY) + ",";
    dataString += String(GyZ) + ",";

    /** Write to the SD card: **/
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      Serial.print(".");
    }
    // if the file isn't open (it really should be), print a debugging error:
    else {
      Serial.println("error opening datalog.txt");
      Serial.println("Fix, then reset Arduino to restart datalogging");
      while(1);
    }
  }
}