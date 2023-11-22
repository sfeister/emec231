/* Acceleration Logger (for Adafruit BNO055)
    Creates data logging CSV file and appends acceleration data periodically

    Built for Arduino Uno

    SD Card Arduino Shield:
      SD Card Shield 3.0 - note the "5V" setting is correct on the switch
      https://wiki.iteadstudio.com/Stackable_SD_Card_shield_V3.0

    Accelerometers:
      Model: Adafruit BNO055
      How to Get Started Using: https://learn.adafruit.com/adafruit-bno055-absolute-orientation-sensor/overview
      Wiring:
        "Vin" on Adafruit BNO055 -> "5 V" on Arduino 
        "GND" on Adafruit BNO055 -> "GND" on Arduino
        "SDA" on Adafruit BNO055 -> "A4" on Arduino
        "SCL" on Adafruit BNO055 -> "A5" on Arduino

    Required Libraries:
    (1) Make sure to install the "Chrono" library from the Arduino Library Manager.
      The Chrono library is for setting timers : http://github.com/SofaPirate/Chrono . 
      We will use the Chrono BasicMetro object: https://github.com/SofaPirate/Chrono/blob/master/examples/basicMetro/basicMetro.ino
    (2) Make sure to install the "Adafruit BNO055" library from the Arduino Library Manager.
      This library connects to our Adafruit BNO055. https://github.com/adafruit/Adafruit_BNO055


    Written by Scott Feister on October 25, 2023.
    Includes code sections copied or modified from various example libraries:
      1. SD card library example: Datalogger.ino  (from the library examples folder)
      2. Adafruit_BNO055 example: read_all_data.ino (from the library examples folder)
      3. Chrono example: basicMetro.ino  https://github.com/SofaPirate/Chrono/blob/master/examples/basicMetro/basicMetro.ino
*/

#define BTN_PIN 2 // optional button
#define REC_PIN 8 // optional recording is live indicator

#include <SPI.h> // For writing to the SD card
#include <SD.h> // For writing to the SD card
#include<Wire.h> // For accessing calibrated data from the Adafruit_BNO055 Accelerometer
#include <Adafruit_Sensor.h> // For accessing calibrated data from the Adafruit_BNO055 Accelerometer
#include <Adafruit_BNO055.h> // For accessing calibrated data from the Adafruit_BNO055 Accelerometer
#include <utility/imumaths.h> // For manipulating data from the Adafruit_BNO055 Accelerometer
#include <Chrono.h> // For setting timers to record data at a measured rate

char buffer[100]; // cstring buffer
int ndots = 0; // printout of dots
const String sep = ",";

// Initialize LED-related variables
uint8_t btn_status; // actually will be boolean

// Initialize SD-related variables
const int chipSelect = 4;
File dataFile;

// Initialize BNO055-related variablles
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
sensors_event_t orientationData, angVelocityData , linearAccelData, magnetometerData, accelerometerData, gravityData;
float AcX,AcY,AcZ,GravX,GravY,GravZ,OrX,OrY,OrZ,GyX,GyY,GyZ;
double QuatW,QuatX,QuatY,QuatZ; // could actually be double
imu::Quaternion myQuaternion;

// Initialize timer-related variables
Chrono myChrono; 
Chrono heartbeatChrono; 
unsigned long deltat;
uint32_t mymillis;

void  setup(){
  myChrono.restart(); // restart / start timer

  /** Initialize LED Pin for camera synchronization **/
  pinMode(BTN_PIN, INPUT);
  pinMode(REC_PIN, OUTPUT);

  /** Initialize serial communication over USB (optional for debugging) **/
  Serial.begin(115200);

  /** Initialise the Adafruit BNO055 sensor **/
  if (!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

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
    // Assemble the file header as a comma-separated list:
    dataFile.println("led,t,deltat,AcX,AcY,AcZ,GravX,GravY,GravZ,QuatW,QuatX,QuatY,QuatZ,OrX,OrY,OrZ,GyX,GyY,GyZ");

    dataFile.close();
  }

  else {
    Serial.println("error opening datalog.txt");
    Serial.println("Fix, then reset Arduino to restart datalogging");
    // don't do anything more:
    while (1);
}

  while (!myChrono.hasPassed(2000)); // wait an even two seconds to start data logging
  myChrono.restart();  // restart the timer
}

void loop(){
  if (myChrono.hasPassed(50)) { // check the timer; has 250 ms elapsed?
    deltat = myChrono.elapsed(); // note the number of milliseconds elapsed since last record (should be close to 250 ms)
    mymillis = millis(); // number of milliseconds since arduino powered on    
    myChrono.restart();  // restart the timer

    /** Grab instantaneous acceleration data from GY-521 **/
    bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
    bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
    bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
    //bno.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
    //bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
    bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);
    myQuaternion = bno.getQuat();
    btn_status = digitalRead(BTN_PIN);

    AcX=linearAccelData.acceleration.x;
    AcY=linearAccelData.acceleration.y;
    AcZ=linearAccelData.acceleration.z;
    GravX=gravityData.acceleration.x;
    GravY=gravityData.acceleration.y;
    GravZ=gravityData.acceleration.z;
    QuatW=myQuaternion.w();
    QuatX=myQuaternion.x();
    QuatY=myQuaternion.y();
    QuatZ=myQuaternion.z();
    OrX=orientationData.orientation.x;
    OrY=orientationData.orientation.y;
    OrZ=orientationData.orientation.z;
    GyX=angVelocityData.gyro.x;  
    GyY=angVelocityData.gyro.y;  
    GyZ=angVelocityData.gyro.z;


    /** Write to the SD card data log file as a comma-separated list: **/
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      //sprintf(buffer,"%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",deltat,AcX,AcY,AcZ,GravX,GravY,GravZ,OrX,OrY,OrZ,GyX,GyY,GyZ);
      dataFile.print(btn_status);
      dataFile.print(sep);
      dataFile.print(mymillis);
      dataFile.print(sep);
      dataFile.print(deltat);
      dataFile.print(sep);
      dataFile.print(AcX);
      dataFile.print(sep);
      dataFile.print(AcY);
      dataFile.print(sep);
      dataFile.print(AcZ);
      dataFile.print(sep);
      dataFile.print(GravX);
      dataFile.print(sep);
      dataFile.print(GravY);
      dataFile.print(sep);
      dataFile.print(GravZ);
      dataFile.print(sep);
      dataFile.print(QuatW,5);
      dataFile.print(sep);
      dataFile.print(QuatX,5);
      dataFile.print(sep);
      dataFile.print(QuatY,5);
      dataFile.print(sep);
      dataFile.print(QuatZ,5);
      dataFile.print(sep);
      dataFile.print(OrX);
      dataFile.print(sep);
      dataFile.print(OrY);
      dataFile.print(sep);
      dataFile.print(OrZ);
      dataFile.print(sep);
      dataFile.print(GyX);
      dataFile.print(sep);
      dataFile.print(GyY);
      dataFile.print(sep);
      dataFile.print(GyZ);
      dataFile.println("");
      dataFile.close();

      Serial.print(F("."));
      ndots++;
      if (ndots > 50){
        Serial.println("");
        ndots = 0;
      }
      if (heartbeatChrono.hasPassed(500) & abs(AcX) > 0.00001) { // check both for the time elapsed and we aren't saving all zeros
        heartbeatChrono.restart();
        digitalWrite(REC_PIN, !digitalRead(REC_PIN)); // toggle heartbeat LED
      }

    }
    // if the file isn't open (it really should be), print a debugging error:
    else {
      Serial.println("error opening datalog.txt");
      Serial.println("Fix, then reset Arduino to restart datalogging");
      while(1);
    }
  }
}