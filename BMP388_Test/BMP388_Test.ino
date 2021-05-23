/*!
  * file bmp388test.ino
  *
  * Connect BMP388 to IIC interface of Arduino, download the program.
  * Altitude is calculated based on temperature and sea level pressure.
  * The example can count an approximate altitude.
  *
  * @n Open the serial monitor, check the altitude.
  * @n Open serial monitor, the temperature could be checked.
  * @n Open serial monitor, the atmospheric pressure could be checked.
  *
  * Copyright   [DFRobot](https://www.dfrobot.com), 2016
  * Copyright   GNU Lesser General Public License
  *
  * version  V0.1
  * date  2018-5-29
  */

#include <M5Stack.h>
#include <WiFi.h>


#include "DFRobot_BMP388.h"
#include "DFRobot_BMP388_I2C.h"
#include "Wire.h"
#include "SPI.h"
#include "math.h"
#include "bmp3_defs.h"

  /*If there is no need to calibrate altitude, comment this line*/
#define CALIBRATE_Altitude

/*Create a bmp388 object to communicate with IIC.*/
DFRobot_BMP388_I2C bmp388;

float seaLevel;

void setup() {

    M5.begin();
    M5.Power.begin();

    /* Initialize the serial port*/
    //Serial.begin(9600);
    /*
     * @brief Set bmp388 IIC address
     * @param BMP3_I2C_ADDR_PRIM: pin SDO is low
     *        BMP3_I2C_ADDR_SEC: pin SDO is high(default)
     */
    bmp388.set_iic_addr(BMP3_I2C_ADDR_SEC);
    /* Initialize bmp388*/
    while (bmp388.begin()) {
        Serial.println("Initialize error!");
        delay(1000);
    }
    /*You can use an accurate altitude to calibrate sea level air pressure.
     *And then use this calibrated sea level pressure as a reference to obtain the calibrated altitude.
     *In this case,525.0m is chendu accurate altitude.
     */
    delay(100);
    seaLevel = bmp388.readSeaLevel(525.0);
    Serial.print("seaLevel : ");
    Serial.print(seaLevel);
    Serial.println(" Pa");
}

void loop() {
#ifdef CALIBRATE_Altitude
    /* Read the calibrated altitude */
    float altitude = bmp388.readCalibratedAltitude(seaLevel);
    Serial.print("calibrate Altitude : ");
    Serial.print(altitude);
    Serial.println(" m");
#else
    /* Read the altitude */
    float altitude = bmp388.readAltitude();
    Serial.print("Altitude : ");
    Serial.print(altitude);
    Serial.println(" m");
#endif
    delay(100);
    /* Read the atmospheric pressure, print data via serial port.*/
    float Pressure = bmp388.readPressure();
    Serial.print("Pressure : ");
    Serial.print(Pressure);
    Serial.println(" Pa");
    delay(100);
    /* Read the temperature, print data via serial port.*/
    float Temperature = bmp388.readTemperature();
    Serial.print("Temperature : ");
    Serial.print(Temperature);
    Serial.println(" C");
    Serial.println();
    delay(1000);
}