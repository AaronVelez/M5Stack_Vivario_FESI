/*
 Name:		Main.ino
 Created:	5/20/2021 10:01:25 PM
 Author:	Aarón I. Vélez Ramírez
*/





//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

////// Board library
#include <M5Stack.h>
//#include <SPIFFS.h>


////// GFX Free Fonts https://rop.nl/truetype2gfx/
#include "FreeSans5pt7b.h"
#include "FreeSans6pt7b.h"
#include "FreeSans7pt7b.h"
#include "FreeSans8pt7b.h"
//#include "FreeSans9pt7b.h" already defined, it can be used
#include "FreeSans10pt7b.h"
//#include "FreeSans12pt7b.h" already defined, it can be used
#include "FreeSans15pt7b.h"
#include "FreeSans20pt7b.h"


////// Credentials_Environmental_Vivario_Lab.h is a user-created library containing paswords, IDs and credentials
#include "Credentials_Environmental_Vivario_Lab.h"
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char iot_server[] = IoT_SERVER;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;
const char iot_data_bucket[] = IoT_DATA_BUCKET;


////// Comunication libraries
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
#define THINGER_SERVER iot_server   // Delete this line if using a free thinger account 
#define _DEBUG_   // Uncomment for debugging connection to Thinger
#define _DISABLE_TLS_     // Uncoment if needed (port 25202 closed, for example)
#include <ThingerESP32.h>
ThingerESP32 thing(iot_user, iot_device, iot_credential);


////// SD fat library used
////// Adapted to M5 stack acording to https://github.com/ArminPP/sdFAT-M5Stack
#include <SdFat.h>
#define SPI_SPEED SD_SCK_MHZ(25)
#define SD_CONFIG SdSpiConfig(TFCARD_CS_PIN, SHARED_SPI, SPI_SPEED) // TFCARD_CS_PIN is defined in M5Stack Config.h (Pin 4)
/// <summary>
/// IMPORTANT
/// in file SdFat\src\SdFatConfig.h at Line 100 set to cero for NON dedicated SPI:
/// #define ENABLE_DEDICATED_SPI 0
/// </summary>
SdFat32 sd;
File32 LogFile;
File32 root;
char line[250];
String str = "";
unsigned int position = 0;


////// Time libraries
#include <TimeLib.h>
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0, 60000); // For details, see https://github.com/arduino-libraries/NTPClient
// Time zone library
#include <Timezone.h>
//  Central Time Zone (Mexico City)
TimeChangeRule mxCDT = { "CDT", First, Sun, Apr, 2, -300 };
TimeChangeRule mxCST = { "CST", Last, Sun, Oct, 2, -360 };
Timezone mxCT(mxCDT, mxCST);


////// Library for SHT31 Temperature and Humidity Sensor
#include <DFRobot_SHT3x.h>
DFRobot_SHT3x sht3x(&Wire,/*address=*/0x44,/*RST=*/4); // secondary I2C address 0x44


////// Library for BMP388 Barometric Pressure Sensor
#include <DFRobot_BMP388.h>
#include <DFRobot_BMP388_I2C.h>
#include <math.h>
#include <bmp3_defs.h>
//#define CALIBRATE_Altitude
DFRobot_BMP388_I2C bmp388;


////// Library for CCS811 Air Quality Sensor
#include <DFRobot_CCS811.h>
DFRobot_CCS811 CCS811;



//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int StaNum = 5;
String StaType = F("Indoor Environmental Monitor");
String StaName = F("Environmental Monitor Vivaro Lab");
String Firmware = F("v1.0.0");
//const float VRef = 3.3;
bool debug = false;


////// Log File & Headers
const char* FileName[] = { "2020.txt", "2021.txt", "2022.txt", "2023.txt", "2024.txt", "2025.txt", "2026.txt", "2027.txt", "2028.txt", "2029.txt",
            "2030.txt", "2031.txt", "2032.txt", "2033.txt", "2034.txt", "2035.txt", "2036.txt", "2037.txt", "2038.txt", "2039.txt",
            "2040.txt", "2041.txt", "2042.txt", "2043.txt", "2044.txt", "2045.txt", "2046.txt", "2047.txt", "2048.txt", "2049.txt",
            "2050.txt" };
const String Headers = F("UNIX_t\tyear\tmonth\tday\thour\tminute\tsecond\t\
AirTemp\tAirRH\t\
Pressure\t\
eCO2ppm\tTVOC\t\
SensorsOK\t\
SentIoT");
const int HeaderN = 14;	// Number of items in header (columns), Also used as a cero-indexed header index
String LogString = "";


////// Time variables
time_t unix_t;	    // RT UNIX time stamp
time_t SD_unix_t;    // Recorded UNIX time stamp
int s = -1;		    // Seconds
int m = -1;		    // Minutes
int h = -1;		    // Hours
int dy = -1;	    // Day
int mo = -1;	    // Month
int yr = -1;	    // Year
// Time for IoT payload
int mIoT = -1;
int hIoT = -1;
int dyIoT = -1;
int moIoT = -1;
int yrIoT = -1;


////// State machine Shift Registers
int LastSec = -1;           // Last second register (to update clock display each second)
int LastLcd = -1;           // Last time the screen was updated
int LastSum = -1;			// Last minute that variables were measured added to the sum for later averaging
int SumNum = 0;				// Number of times a variable value has beed added to the sum for later averaging
int LastLog = -1;			// Last minute that variables were loged to the SD card
bool PayloadRdy = false;	// Payload ready to send to IoT

byte SensorsOK = B00000000;     // Byte variable to store real time sensor status
byte SensorsOKAvg = B00011111;  // Byte variable to store SD card average sensor status
int SensorsOKIoT = 0;           // Variable to send sensor status in decimal format

time_t t_DataBucket = 0;             // Last time Data was sent to bucket (in UNIX time format) 
const int DataBucket_frq = 150;       // Data bucket update frequency in seconds (must be more than 60)


////// Measured instantaneous variables
float Temp = -1;        // Air temperature 
float RH = -1;          // Air RH value 
float Pressure = -1;    // Barometric presure 
float CO2ppm = -1;      // eCO2 value 
float TVOC = -1;        // Total Volatile Organic Compounds (TVOC) value 


////// Variables to store sum for eventual averaging
double TempSum = 0;
double RHSum = 0;
double PressureSum = 0;
double CO2ppmSum = 0;
double TVOCSum = 0;


////// Values to be logged. They will be the average over the last 5 minutes
float TempAvg = 0;
float RHAvg = 0;
float PressureAvg = 0;
float CO2ppmAvg = 0;
float TVOCAvg = 0;


////// Variables to store value-coded color
int TempRGB = 0;
int RHRGB = 0;
int PressureRGB = 0;
int CO2RGB = 0;
int TVOCRGB = 0;



//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));
    
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    M5.Lcd.print(F("Connecting to internet..."));
    // Test for half a minute if there is WiFi connection;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 30; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("Connected to internet!"));
            break;
        }
        if(i == 30) { M5.Lcd.println(F("No internet connection")); }
    }
    /*
    // Setup SPIFFS Flash file system. See https://github.com/me-no-dev/arduino-esp32fs-plugin 
    M5.Lcd.print(F("Starting SPIFFS..."));
    if (!SPIFFS.begin(true)) {
        M5.Lcd.println(F("SPIFFS Mount Failed"));
        return;
    }
    */

    ////// Configure IoT
    M5.Lcd.println(F("Configuring IoT..."));
    thing.add_wifi(ssid, password);
    // Define input resources
    thing["Debug"] << [](pson& in) {
        if (in.is_empty()) { in = debug; }
        else { debug = in; }
    };
    
    // Define output resources
    thing["RT_Temp"] >> [](pson& out) { out = Temp; };
    thing["RT_RH"] >> [](pson& out) { out = RH; };
    thing["RT_Barometric_Pressure"] >> [](pson& out) { out = Pressure; };
    thing["RT_eCarbon_Dioxide"] >> [](pson& out) { out = CO2ppm; };
    thing["RT_TVOC"] >> [](pson& out) { out = TVOC; };
    thing["RT_SensorOK"] >> [](pson& out) { out = SensorsOK; };

    thing["Avg_Data"] >> [](pson& out) {
        out["Time_Stamp"] = SD_unix_t;
        out["Temperature"] = TempAvg;
        out["Relative_Humidity"] = RHAvg;
        out["Barometric_Pressure"] = PressureAvg;
        out["eCarbon_Dioxide"] = CO2ppmAvg;
        out["TVOC"] = TVOCAvg;
        out["Sensors_OK"] = SensorsOKIoT;
    };
    

    ////// Initialize SD card
    M5.Lcd.println(F("Setting SD card..."));
    sd.begin(SD_CONFIG);
    // Reserve RAM memory for large and dynamic String object
    // used in SD file write/read
    // (prevents heap RAM framgentation)
    LogString.reserve(HeaderN * 7);
    str.reserve(HeaderN * 7);
    

    //////// Start NTP client engine and update system time
    M5.Lcd.println(F("Starting NTP client engine..."));
    timeClient.begin();
    M5.Lcd.println(F("Trying to update NTP time..."));
    // Test for half a minute if NTP time can be updated;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 30; i++) {
        if (!timeClient.forceUpdate()) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("NTP time updated!"));
            break;
        }
        if (i == 30) { M5.Lcd.println(F("No NTP update")); }
    }
    unix_t = timeClient.getEpochTime();
    unix_t = mxCT.toLocal(unix_t); // Conver to local time
    setTime(unix_t);   // set system time to given unix timestamp

    
    // Start SHT31 Temp and RH sensor
    M5.Lcd.println(F("Starting Temp/RH sensor..."));
    if (sht3x.begin() != 0) {
        M5.Lcd.println(F("Failed to initialize the chip, please confirm the wire connection"));
        M5.Lcd.println(F("Make sure the I2C address selector is 0x44"));
        delay(1000);
    }
    M5.Lcd.print(F("Chip serial number: "));
    M5.Lcd.println(sht3x.readSerialNumber());
    if (!sht3x.softReset()) {
        M5.Lcd.println(F("Failed to initialize the chip...."));
        delay(1000);
    }

    
    // Start BMP388 Barometric Pressure sensor
    M5.Lcd.println(F("Starting barometric pressure sensor..."));
    bmp388.set_iic_addr(BMP3_I2C_ADDR_SEC); // default I2C address 0x77
    if (bmp388.begin() != 0) {
        M5.Lcd.println(F("Failed to initialize the chip, please confirm the wire connection"));
        delay(1000);
    }
    
    
    // Start CCS811 Air Quality sensor
    M5.Lcd.println(F("Starting air quality sensor..."));
    if (CCS811.begin() != 0) {
        M5.Lcd.println(F("Failed to initialize the chip, please confirm the wire connection"));
        delay(1000);
    }
    CCS811.setMeasurementMode(CCS811.eCycle_1s, 0, 0);  // constant power mode, read evry second is possible
    

    // Setup finish message
    M5.Lcd.println(F("Setup done!"));
    M5.Lcd.println(F("\nStarting in "));
    for (int i = 0; i <= 5; i++) {
        M5.Lcd.println(5 - i);
        if (i != 5) { delay(1000); }
        else { delay(250); }
    }
    M5.Lcd.clear(BLACK);
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    if (debug) {
        M5.Lcd.clear(BLACK);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.setFreeFont(&FreeSans6pt7b);
        M5.Lcd.print(F("Loop start at: "));
        M5.Lcd.println(millis());
    }
    ////// State 1. Keep the Iot engine runing
    thing.handle();
    

    ////// State 2. Get current time from system time
    if (true) {
        s = second();
        m = minute();
        h = hour();
        dy = day();
        mo = month();
        yr = year();
        if (debug) { M5.Lcd.println((String)"Time: " + h + ":" + m + ":" + s); }
    }


    ////// State 3. Update system time from NTP server every minute
    if (timeClient.update()) { // It does not force-update NTC time (see NTPClient declaration for actual udpate interval)
        unix_t = timeClient.getEpochTime();
        unix_t = mxCT.toLocal(unix_t); // Conver to local time
        setTime(unix_t);   // set system time to given unix timestamp
        if (debug) { M5.Lcd.println(F("NTP client update success!")); }
    }
    else {
        if (debug) { M5.Lcd.println(F("NTP update not succesfull")); }
    }



    ////// State 4. Test if it is time to read sensor values (each 15 seconds)
    if ( (s % 15 == 0) && (s != LastSum) ) {
        // SHT31 Temp and RH
        Temp = sht3x.getTemperatureC();
        if (isnan(Temp) || Temp == -1) {    // Check Temp values
            Temp = 25;
            bitWrite(SensorsOK, 0, 0);
        }
        else { bitWrite(SensorsOK, 0, 1); }
        RH = sht3x.getHumidityRH();
        if (isnan(RH) || RH == -1) {      // Check RH values
            RH = 25;
            bitWrite(SensorsOK, 1, 0);
        }
        else { bitWrite(SensorsOK, 1, 1); }
        if (debug) {
            M5.Lcd.println((String)"Temp: " + Temp + " °C");
            M5.Lcd.println((String)"RH: " + RH + " %");
        }
        
        // BMP388 Barometric pressure
        Pressure = bmp388.readPressure() / 1000;            // in kPa
        //Pressure = bmp388.readPressure() * 0.00750062;    // in mmHg
        //Pressure = bmp388.readPressure() * 0.00014504;    // in psi
        //Pressure = bmp388.readPressure() / 101325;        // in atm
        if (isnan(Pressure) || Pressure == -1) {      // Check Pessure values
            RH = 760 * 0.77;   // Aprox presure at Mexico City in mmHg
            bitWrite(SensorsOK, 2, 0);
        }
        else { bitWrite(SensorsOK, 2, 1); }
        if (debug) { M5.Lcd.println((String)"Pressure: " + Pressure + " kPa"); }

        
        // CCS811 Air Quality
        if (CCS811.checkDataReady() == true) {
            CO2ppm = CCS811.getCO2PPM();
            bitWrite(SensorsOK, 3, 1);
            TVOC = CCS811.getTVOCPPB();
            bitWrite(SensorsOK, 4, 1);
        }
        else {              // Set values to -1 to identify sensor fail
            CO2ppm = 400;
            bitWrite(SensorsOK, 3, 0);
            TVOC = 0;
            bitWrite(SensorsOK, 4, 0);
        }
        // Set Temp and RH (measured by SFT31) to compensate future CCS811 readings
        if ( bitRead(SensorsOK, 0) && bitRead(SensorsOK, 1) ) {     // Read if Temp and RH sensor values are OK
            CCS811.setInTempHum(Temp, RH);
        }
        if (debug) {
            M5.Lcd.println((String)"eCO2: " + CO2ppm + " ppm");
            M5.Lcd.println((String)"TVOC: " + TVOC + " ppb");
        }


        // Record if sensor reads were OK
        SensorsOKAvg = SensorsOKAvg & SensorsOK;
        if (debug) {
            if (!bitRead(SensorsOK, 0) || !bitRead(SensorsOK, 1) || !bitRead(SensorsOK, 2) ||
                !bitRead(SensorsOK, 3) || !bitRead(SensorsOK, 4)) {
                M5.Lcd.println(F("At least 1 sensor read failed"));
                M5.Lcd.print(F("SensorOK byte: "));
                M5.Lcd.println(SensorsOK, BIN);
                M5.Lcd.print(F("Temp: "));
                M5.Lcd.println(Temp);
                M5.Lcd.print(F("RH: "));
                M5.Lcd.println(RH);
                M5.Lcd.print(F("Pressure: "));
                M5.Lcd.println(Pressure);
                M5.Lcd.print(F("eCO2: "));
                M5.Lcd.println(CO2ppm);
                M5.Lcd.print(F("TVOC: "));
                M5.Lcd.println(TVOC);
            }
        }
        
        // Add new values to sum        
        TempSum += Temp;
        RHSum += RH;
        PressureSum += Pressure;
        CO2ppmSum += CO2ppm;
        TVOCSum += TVOC;

        // Update Shift registers
        LastSum = s;
        SumNum += 1;
    }


    ////// State 5. Test if it is time to compute  averages and record in SD card (each 5 minutes)
    if ( ((m % 5) == 0) && (m != LastLog) && (SumNum > 0) ) {
        // Calculate averages
        TempAvg = TempSum / SumNum;
        RHAvg = RHSum / SumNum;
        PressureAvg = PressureSum / SumNum;
        CO2ppmAvg = CO2ppmSum / SumNum;
        TVOCAvg = TVOCSum / SumNum;
        


        // Open Year LogFile (create if not available)
        if (!sd.exists(FileName[yr - 2020])) {
            LogFile.open((FileName[yr - 2020]), O_RDWR | O_CREAT); // Create file

            // Add Metadata
            LogFile.println(F("Start position of last line send to IoT:\t1"));
            // Tabs added to prevent line ending with 0. Line ending with 0 indicates that line needs to be sent to IoT.
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("Metadata:"));
            LogFile.println((String)"Station Number\t" + StaNum + "\t\t\t");
            LogFile.println((String)"Station Name\t" + StaName + "\t\t\t");
            LogFile.println((String)"Station Type\t" + StaType + "\t\t\t");
            LogFile.println((String)"Firmware\t" + Firmware + "\t\t\t");
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));

            LogFile.println(Headers); // Add Headers
        }
        else {
            LogFile.open(FileName[yr - 2020], O_RDWR); // Open file
            LogFile.seekEnd(); // Set position to end of file
        }


        // Log to SD card
        LogString = (String)unix_t + "\t" + yr + "\t" + mo + "\t" + dy + "\t" + h + "\t" + m + "\t" + s + "\t" +
            String(TempAvg, 4) + "\t" + String(RHAvg, 4) + "\t" +
            String(Pressure, 4) + "\t" +
            String(CO2ppmAvg, 0) + "\t" + String(TVOCAvg, 0) + "\t" +
            String(SensorsOKAvg, DEC) + "\t" +
            "0";
        LogFile.println(LogString); // Prints Log string to SD card file "LogFile.txt"
        LogFile.close(); // Close SD card file to save changes


        // Reset Shift Registers
        LastLog = m;

        TempSum = 0;
        RHSum = 0;
        PressureSum = 0;
        CO2ppmSum = 0;
        TVOCSum = 0;

        SumNum = 0;
        SensorsOKAvg = B00011111;
    }


    ////// State 6. Test if there is data available to be sent to IoT cloud
    // Only test if Payload is not ready AND 
    // the next DataBucket upload oportunity is in 15 sec
    if (!PayloadRdy &&
        unix_t - t_DataBucket > DataBucket_frq - 15) {
        root.open("/");	// Open root directory
        root.rewind();	// Rewind root directory
        LogFile.openNext(&root, O_RDWR);
        while (LogFile.openNext(&root, O_RDWR)) {
            LogFile.rewind();
            LogFile.fgets(line, sizeof(line));     // Get first line
            str = String(line);
            if (debug) {
                M5.Lcd.print(F("File first line: "));
                M5.Lcd.println(str.substring(0, str.indexOf("\r")));
            }
            str = str.substring(str.indexOf("\t"), str.indexOf("\r"));
            if (str == "Done") {	// Skips file if year data is all sent to IoT
                LogFile.close();
                continue;
            }
            position = str.toInt();	// Sets file position to start of last line sent to IoT
            LogFile.seekSet(position);	// Set position to last line sent to LoRa
            // Read each line until a line not sent to IoT is found
            while (LogFile.available()) {
                position = LogFile.curPosition();  // START position of current line
                int len = LogFile.fgets(line, sizeof(line));
                if (line[len - 2] == '0') {
                    str = String(line); // str is the payload, next state test if there is internet connection to send payload to IoT
                    if (debug) {
                        M5.Lcd.println("Loteria, data to send to IoT found!");
                        M5.Lcd.print(F("Data: "));
                        M5.Lcd.println(str.substring(0, str.indexOf("\r")));
                    }
                    PayloadRdy = true;
                    break;
                }
            }
        }
        root.close();
        LogFile.close();
    }


    ////// State 10. Test if there is Internet and a Payload to sent SD data to IoT
    if (debug) {
        M5.Lcd.print(F("Thing connected, payload ready and enought time has enlapsed: "));
        M5.Lcd.println(thing.is_connected() &&
            PayloadRdy &&
            unix_t - t_DataBucket > DataBucket_frq);
    }
    if (thing.is_connected() &&
        PayloadRdy &&
        unix_t - t_DataBucket > DataBucket_frq) {
        t_DataBucket = unix_t; // Record Data Bucket update TRY; even if it is not succesfful
        // extract data from payload string (str)
        for (int i = 0; i < HeaderN; i++) {
            String buffer = str.substring(0, str.indexOf('\t'));
            if (i != 6) { 	// Do not read seconds info
                if (i == 0) {   // UNIX Time
                    SD_unix_t = buffer.toInt();
                }
                else if (i == 1) {
                    yrIoT = buffer.toInt();
                }
                else if (i == 2) {
                    moIoT = buffer.toInt();
                }
                else if (i == 3) {
                    dyIoT = buffer.toInt();
                }
                else if (i == 4) {
                    hIoT = buffer.toInt();
                }
                else if (i == 5) {
                    mIoT = buffer.toInt();
                }
                else if (i == 7) {  // Temp
                    TempAvg = buffer.toFloat();
                }
                else if (i == 8) {  // RH
                    RHAvg = buffer.toFloat();
                }
                else if (i == 9) {  // Pressure
                    PressureAvg = buffer.toFloat();
                }
                else if (i == 10) { // eCO2
                    CO2ppmAvg = buffer.toFloat();
                }
                else if (i == 11) { // TVOC
                    TVOCAvg = buffer.toFloat();
                }
                else if (i == 12) { // SensorsOK
                    SensorsOKIoT = buffer.toInt();
                }
            }
            str = str.substring(str.indexOf('\t') + 1);
        }
        
        // send data to IoT. If succsessful, rewrite line in log File
        if (thing.write_bucket(iot_data_bucket, "Avg_Data", true) == 1) {
            if (debug) { M5.Lcd.println(F("Loteria, data on Cloud!!!")); }
            // Update line sent to IoT status
            if (debug) {
                M5.Lcd.print(F("IoT year: "));
                M5.Lcd.println(yrIoT);
                M5.Lcd.print(F("File name: "));
                M5.Lcd.println(FileName[yrIoT - 2020]);
            }
            LogFile.open(FileName[yrIoT - 2020], O_RDWR); // Open file containing the data just sent to IoT
            str = String(line);                     // Recover complete payload from original line
            str.setCharAt(str.length() - 2, '1');   // Replace 0 with 1, last characters are always "\r\n"
            LogFile.seekSet(position);              // Set position to start of line to be rewritten
            LogFile.println(str.substring(0, str.length() - 1));    // Remove last character ('\n') to prevent an empty line below rewritten line
            
            // Test if this line is last line of year Log File, if so, write "Done" at the end of first line
            if (moIoT == 12 && dyIoT == 31 && hIoT == 23 && mIoT == 55) {
                LogFile.rewind();
                LogFile.fgets(line, sizeof(line));     // Get first line
                str = String(line);
                str = str.substring(0, str.indexOf("\t"));
                str = (String)str + "\t" + "Done";
                LogFile.rewind();
                LogFile.println(str.substring(0, str.length() - 1));    // Remove last character ('\n') to prevent an empty line below rewritten line
                LogFile.close();
            }
            else {
                // Update start position of last line sent to IoT
                LogFile.rewind();
                LogFile.fgets(line, sizeof(line));     // Get first line
                str = String(line);
                str = str.substring(0, str.indexOf("\t"));
                str = (String)str + "\t" + position;
                LogFile.rewind();
                LogFile.println(str);
                LogFile.close();
            }
            PayloadRdy = false;
        }

    }

    
    ////// State 7. Update Screen
    if ( !debug && (s % 10 == 0) && (s != LastLcd) ) {
        // Backgound colors
        TempRGB = constrain(round((Temp * 255) / /*max expected value*/ 50), 64, 255);
        RHRGB = constrain(round((RH * 255) / /*max expected value*/ 100), 64, 255);
        PressureRGB = constrain(round((Pressure * 255) / /*max expected value*/ 700), 64, 255);
        CO2RGB = constrain(round((CO2ppm * 255) / /*max expected value*/ 2500), 64, 255);
        TVOCRGB = constrain(round((TVOC * 255) / /*max expected value*/ 1000), 64, 255);

        // 1. Temp
        M5.Lcd.fillRect(0, 0, 320, (40 * 1), M5.Lcd.color565(TempRGB, 0, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255 - TempRGB, 255, 255 - TempRGB));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(F("Temp. aire"), 10, (40 * 1) - 20);
        M5.Lcd.drawString(String(Temp), 160, (40 * 1) - 20);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.drawString(F("C"), 300, (40 * 1) - 20);
        
        // 2. RH
        M5.Lcd.fillRect(0, (40 * 1), 320, (40 * 2), M5.Lcd.color565(0, 0, RHRGB));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255 - RHRGB));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(F("HR aire"), 10, (40 * 2) - 20);
        M5.Lcd.drawString(String(RH), 160, (40 * 2) - 20);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.drawString(F("%"), 300, (40 * 2) - 20);

        // 3. Pressure
        M5.Lcd.fillRect(0, (40 * 2), 320, (40 * 3), M5.Lcd.color565(PressureRGB, PressureRGB, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255 - PressureRGB, 255 - PressureRGB, 255));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(F("Presion atm."), 10, (40 * 3) - 20);
        M5.Lcd.drawString(String(Pressure), 160, (40 * 3) - 20);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.drawString(F("mmHg"), 300, (40 * 3) - 20);

        // 4. eCO2
        M5.Lcd.fillRect(0, (40 * 3), 320, (40 * 4), M5.Lcd.color565(CO2RGB, CO2RGB, CO2RGB));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255 - CO2RGB, 255 - CO2RGB));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(F("eCO2"), 10, (40 * 4) - 20);
        M5.Lcd.drawString(String(CO2ppm, 0), 160, (40 * 4) - 20);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.drawString(F("ppm"), 300, (40 * 4) - 20);
        
        // 5. TVOC
        M5.Lcd.fillRect(0, (40 * 4), 320, (40 * 5), M5.Lcd.color565(TVOCRGB, 0, TVOCRGB));
        M5.Lcd.setTextColor(M5.Lcd.color565(255 - TVOCRGB, 255, 255 - TVOCRGB));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(F("TVOC"), 10, (40 * 5) - 20);
        M5.Lcd.drawString(String(TVOC, 0), 160, (40 * 5) - 20);
        M5.Lcd.setTextDatum(MR_DATUM);
        M5.Lcd.drawString(F("ppb"), 300, (40 * 5) - 20);

        LastLcd = s;
    }
    

    
    ///// State 8. Update clock and IoT conection display
    if (!debug && (s != LastSec)) {
        M5.Lcd.fillRect(0, 200, 320, 240, M5.Lcd.color565(0, 0, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString((String)dy + "/" + mo + "/" + yr + "  " + h + ":" + m + ":" + s, 10, 220);

        // IoT connection
        if (thing.is_connected()) {
            M5.Lcd.setFreeFont(&FreeSans12pt7b);
            M5.Lcd.setTextDatum(MR_DATUM);
            M5.Lcd.drawString(F("IoT"), 320 - 10, 220);
        }

        LastSec = s;
    }
    

    if (debug) {
        M5.Lcd.print(F("Loop end at: "));
        M5.Lcd.println(millis());
        M5.Lcd.println(F("Press button A to continue"));
        while (true) {
            thing.handle(); // Keep IoT engine runningat all times
            M5.update();    // Update buyyom state
            if (M5.BtnA.wasPressed()) {
                break;
            }
        }
    }
}
