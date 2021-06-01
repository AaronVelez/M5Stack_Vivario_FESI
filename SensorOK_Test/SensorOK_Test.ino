/*
 Name:		SensorOK_Test.ino
 Created:	5/29/2021 9:48:23 PM
 Author:	aivel
*/

byte SensorsOK = 0b00000000;    // Byte variable to store real time sensor status
byte SensorsOKAvg = 0b00011111; // Byte variable to store SD card average sensor status


// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	delay(500);
	Serial.print(F("SensorOK original: "));
	Serial.println(SensorsOK, BIN);
	Serial.print(F("SensorOKAvg original: "));
	Serial.println(SensorsOKAvg, BIN);
	bitWrite(SensorsOK, 1, 1);
	Serial.print(F("SensorOK modified: "));
	Serial.println(SensorsOK, BIN);
	SensorsOKAvg = SensorsOKAvg & SensorsOK;
	Serial.print(F("SensorOKAvg after bit operation: "));
	Serial.println(SensorsOKAvg, BIN);

}

// the loop function runs over and over again until power down or reset
void loop() {

  
}
