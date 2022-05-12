#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

#define readPin1 A0                                         // Naming the arduino pin A0.
#define readPin2 A1                                         // Naming the arduino pin A1.
float refVolt = 4983;                                       // Varies for each arduino and has to be measured on the device.
float maxVolt = refVolt * 0.9;                              // Input volt at max pressure.
float minVolt = refVolt * 0.1;                              // Input volt at min pressure.
float vPerUnit = refVolt / (1024);                          // Volt/Unit (units is arduinos scale for the analogRead command).
float paPerMV = 2500 / (maxVolt - minVolt);                 // Pascal/milliVolt.
float mV1 = 0;                                              // Holds the latest mV.
float mV2 = 0;                                              // Holds the latest mV.
float pa1 = 0;                                              // Holds the latest mV.
float pa2 = 0;                                              // Holds the latest mV.
float paOffset = 15.26;                                     // Since the sensor is not ideal, an offset is needed.

int RXPin = 3;                                              // Choose two digital Arduino pins for GSP
int TXPin = 2;                                              // Choose two digital Arduino pins for GSP
SoftwareSerial gpsSerial(RXPin, TXPin);                     // Create a software serial port called "gpsSerial"
String gpsData[15];                                         // Stores the latest gps data.

String fileName = "log";                                    // Name of the log file
String labels = "%Time, Pa1, Pa2, mV1, mV2, Lat, Lng";      // Headers for log file.




//-----------------------------------------------------------------------------------------------------------
void setup() {
  SDinit();                                                 // Initialing SD card reader
  gpsSerial.begin(9600);                                    // Default baud of NEO-6M is 9600
  Serial.begin(9600);
  Serial.println("Initialization complete");
  Serial.end();
}

void loop() {
  logg();                                                   // Logging current data to log file
}
//-----------------------------------------------------------------------------------------------------------




//-------------------------------------------- Functions ----------------------------------------------------
float voltOnPin(int pin) {
  return analogRead(pin) * vPerUnit;                        // Converts analogRead() units to millivolt.
}
//-----------------------------------------------------------------------------------------------------------
float vToPa(float mV_) {
  return ((mV_ - minVolt) * paPerMV) - 1250;                // Converts millivolt to pascall.
}
//-----------------------------------------------------------------------------------------------------------
String readNMEA() {
  while (gpsSerial.available() > 0) {
    gpsSerial.read();                                       // Reading data recieved on the RX pin (pin 3)
  }
  if (gpsSerial.find("$GPRMC,")) {                          // Looking in the gps data until "$GPRMC," is found
    String tmp = gpsSerial.readStringUntil('\n');           // Saves the data between "$GPRMC," and a new line
    return tmp;                                             // Returning NMEA data

  } else {
    Serial.begin(9600);                                     // Printing error if no NMEA data was found
    Serial.println("No nmea data found");
    Serial.end();
  }
}
//-----------------------------------------------------------------------------------------------------------
void parseNMEA(String tempMsg) {
  int pos = 0;                                              // Variable index
  int stringplace = 0;                                      // Points to were the variable in the gps data starts
  for (int i = 0; i < tempMsg.length(); i++) {              // Looping through the saved gps data
    if (tempMsg.substring(i, i + 1) == ",") {               // Looking for a "," (vars is sepeared by a "," in the data)
      gpsData[pos] = tempMsg.substring(stringplace, i);     // Croping the vars from the data and storing them in the gpsData array
      stringplace = i + 1;                                  // Moving the pointer of the next variable
      pos++;                                                // Increasing variable index
    }
  }
  gpsData[2] = ConvertLat();                                // Converting lat to DD format
  gpsData[4] = ConvertLng();                                // Converting lng to DD format
  gpsData[0] = gpsData[0].toInt() + 20000;                  // Converting time to swedish timezone
}
//-----------------------------------------------------------------------------------------------------------
String ConvertLat() {
  String posneg = "";                                       // Checks if we are in the southern hemosphere
  if (gpsData[3] == "S") {                                  // Checks if we are in the southern hemosphere
    posneg = "-";                                           // Checks if we are in the southern hemosphere
  }

  String latfirst;                                          // Stores the latitude part
  float latsecond;                                          // Stores the degree part
  for (int i = 0; i < gpsData[2].length(); i++) {           // Looping through the latitude variable (gps format)
    if (gpsData[2].substring(i, i + 1) == ".") {            // Searching for the dot in the degrees
      latfirst = gpsData[2].substring(0, i - 2);            // The degrees has to digits before the det
      latsecond = gpsData[2].substring(i - 2).toFloat();    // The rest of the string is degrees, converting it to float
    }
  }
  latsecond = latsecond / 60;                               // Dividing the degrees with 60 to get it to LongLat format

  String CalcLat = "";                                      // Creating a string to return
  char charVal[9];                                          // Creating a character array
  dtostrf(latsecond, 4, 6, charVal);                        // Converting float to characters and storing it in the char array
  for (int i = 0; i < sizeof(charVal); i++) {               // Looping through character array
    CalcLat += charVal[i];                                  // Adding the characters to a string
  }
  latfirst += CalcLat.substring(1);                         // Combining the lat value with the converted degrees
  latfirst = posneg += latfirst;                            // Adding negative if in southern hemposphere
  return latfirst;                                          // Returning the converted lat value
}
//-----------------------------------------------------------------------------------------------------------
String ConvertLng() {
  String posneg = "";
  if (gpsData[5] == "W") {
    posneg = "-";
  }
  String lngfirst;
  float lngsecond;
  for (int i = 0; i < gpsData[4].length(); i++) {
    if (gpsData[4].substring(i, i + 1) == ".") {
      lngfirst = gpsData[4].substring(0, i - 2);
      lngsecond = gpsData[4].substring(i - 2).toFloat();
    }
  }
  lngsecond = lngsecond / 60;
  String CalcLng = "";
  char charVal[9];
  dtostrf(lngsecond, 4, 6, charVal);
  for (int i = 0; i < sizeof(charVal); i++) {
    CalcLng += charVal[i];
  }
  lngfirst += CalcLng.substring(1);
  lngfirst = posneg += lngfirst;
  return lngfirst;
}
//-----------------------------------------------------------------------------------------------------------
void logg() {
  String nmea = readNMEA();                                 // Reading NMEA sentenses.
  parseNMEA(nmea);                                          // Converting NMEA sentence to readable gps data.
  mV1 = voltOnPin(readPin1);                                // Retrives the current voltage on the analog pin 1.
  mV2 = voltOnPin(readPin2);                                // Retrives the current voltage on the analog pin 2.
  pa1 = vToPa(mV1) + paOffset;                              // Converting voltage to pascal.
  pa2 = vToPa(mV2) + paOffset;                              // Converting voltage to pascal.
  Save();
}
//-----------------------------------------------------------------------------------------------------------
void newFileName() {
  bool file = false;                                        // Tracks when we found a file name we can use
  int count = 0;                                            // Filename index
  while (!file) {                                           // Loops until new file name has been found
    if (SD.exists(fileName + ".csv")) {                     // Checks if current name is new
      count++;                                              // If it name already exists, increment name
      fileName = "log" + (String)count;                     // Make a new name
    }
    else file = true;                                       // Exits loop when a new name is found
  }
  File myFile = SD.open((fileName + ".csv"), FILE_WRITE);   // Creating a log file with the new file name
  if (myFile) {                                             // Checks if file opened succesfully
    myFile.println(labels);                                 // Printing file headers
    myFile.close();                                         // Closing the log file
  }
  else {                                                    // Printing error msg if file didn't open
    Serial.begin(9600);
    Serial.println("error: SDinit failed");
    Serial.end();
  }
}
//-----------------------------------------------------------------------------------------------------------
void SDinit() {
  if (!SD.begin(5)) {                                       // Checking if card has been initialized
    return;
  }
  newFileName();
}
//-----------------------------------------------------------------------------------------------------------
void Save() {
  File dataFile = SD.open((fileName + ".csv"), FILE_WRITE);     // Oppening current log file
  if (dataFile) {                                               // Checks if file opened succesfully
    dataFile.print(gpsData[0]);
    dataFile.print(String(pa1) + ",");
    dataFile.print(String(pa2) + ",");
    dataFile.print(String(mV1) + ",");
    dataFile.print(String(mV2) + ",");
    dataFile.print(gpsData[2] + ",");
    dataFile.println(gpsData[4]);
    dataFile.close();                                           // Closing the log file

    Serial.begin(9600);
    Serial.print("Logfile: " + fileName + "   ");
    Serial.print("Time: " + gpsData[0]);
    Serial.print("P1: " + String(pa1) + "   ");
    Serial.print("P2: " + String(pa2) + "   ");
    Serial.print("V1: " + String(mV1) + "   ");
    Serial.print("V2: " + String(mV2) + "   ");
    Serial.print("Lat: " + gpsData[2] + "   ");
    Serial.println("Lng: " + gpsData[4]);
    Serial.end();
  }
  else {                                                        // Printing error msg if log file didn't open
    Serial.begin(9600);
    Serial.println("SD card error");
    Serial.end();
  }
}
