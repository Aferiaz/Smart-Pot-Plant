#include<EEPROM.h>

#define SoilMPIN A0

void setup() {
  Serial.begin(9600);
  pinMode(SoilMPIN, INPUT);
  pinMode(3, INPUT_PULLUP);
  Serial.println("Welcome to the Soil Moisture Sensor Calibrator");
  Serial.println("Dry Reading: Please make sure the Soil Moisture Sensor is DRY");
  Serial.println("Please Press Button to Continue");
  while (digitalRead(3)) {
  }
  Serial.println("Placing Dry Reading on EEPROM...");
  delay(1000);
  int DryRead = analogRead(SoilMPIN);
  Serial.print("Dry: ");
  Serial.println(DryRead);
  EEPROM.put(2, DryRead);
  
  Serial.println();
  
  Serial.println("Wet Reading: Please submerge 3/4 of the Soil Moisture Sensor in water");
  Serial.println("Please Press Button to Continue");
  while (digitalRead(3)) {
  }
  Serial.println("Placing Wet Reading on EEPROM...");
  delay(1000);
  int WetRead = analogRead(SoilMPIN);
  Serial.print("Wet: ");
  Serial.println(WetRead);
  EEPROM.put(4, WetRead);
  
  Serial.println();
  Serial.println("You may now use the SmartPot Program :)");
  
}

void loop() {
}
