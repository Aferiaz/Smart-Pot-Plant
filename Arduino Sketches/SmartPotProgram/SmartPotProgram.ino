#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define DHTPIN 8
#define DHTTYPE DHT11
#define SoilMPIN A0
#define countof(a) (sizeof(a) / sizeof(a[0]))
#define chipSelect 10

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd (0x27, 16, 2);
ThreeWire myWire(4, 5, 2);
RtcDS1302<ThreeWire> Rtc(myWire);

int wetm, drym;
float humidity, temp;
int moistureP, moistureV;
uint64_t tempsec1, tempsec2;
bool SDInserted = 0, backlightON = 1;
File dataFile;

volatile unsigned long last_micros;

byte soilmSymb[] = {
  B00100,
  B01110,
  B01010,
  B01010,
  B11111,
  B01010,
  B00000,
  B00000
};

byte tempSymb[] = {
  B11100,
  B01000,
  B01000,
  B00000,
  B10011,
  B00100,
  B00100,
  B00011
};

byte humiditySymb[] = {
  B00000,
  B00100,
  B00100,
  B01110,
  B11111,
  B11111,
  B01110,
  B00000
};

byte SDOn[] = {
  B00000,
  B00000,
  B01100,
  B01110,
  B01110,
  B01110,
  B00000,
  B00000
};

byte SDOff[] = {
  B00000,
  B11111,
  B10011,
  B10001,
  B10001,
  B10001,
  B11111,
  B00000
};

void setup() {
  pinMode(SoilMPIN, INPUT);
  pinMode(3, INPUT_PULLUP);
  dht.begin();
  lcd.init();
  lcd.backlight();
  attachInterrupt(digitalPinToInterrupt(3), pin3_pressed, FALLING);
  EEPROM.get(2, drym);
  EEPROM.get(4, wetm);
  checkRTC();
  if (!SD.begin(chipSelect)) {
    SDInserted = 0;
    lcd.setCursor(0, 0);
    lcd.print("No SD Card Found");
    delay(3000);
    lcd.clear();
  }
  else {
    SDInserted = 1;
    lcd.setCursor(0, 0);
    lcd.print("SD Card Found");
    delay(2000);
    lcd.clear();
    if (!SD.exists("PotData.txt")) {
      dataFile = SD.open("PotData.txt", FILE_WRITE);
      dataFile.println("Date & Time\tSoil Moisture (%)\tHumidity (%)\tTemperature (°C)");
      dataFile.flush();
    }
    else dataFile = SD.open("PotData.txt", FILE_WRITE);
  }
  RtcDateTime now = Rtc.GetDateTime();
  tempsec1 = now.TotalSeconds64();
  tempsec2 = now.TotalSeconds64();
}

void loop() {
  if (backlightON) lcd.backlight();
  else lcd.noBacklight();
  RtcDateTime now = Rtc.GetDateTime();
  if (!Rtc.IsDateTimeValid()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC is not conn-");
    lcd.setCursor(0, 1);
    lcd.print("ected properly!");
    while (!Rtc.IsDateTimeValid());
    lcd.clear();
  }
  if (now.TotalSeconds64() - tempsec1 >= 1) {
    moistureV = analogRead(SoilMPIN);
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    moistureP = map(moistureV, wetm, drym, 100, 0);
    if (isnan(temp) || isnan(humidity)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DHT Sensor");
      lcd.setCursor(0, 1);
      lcd.print("not read");
      delay(2000);
      lcd.clear();
      return;
    }
    LCDDateTime(now);
    lcdPrint(moistureP, temp, humidity);

    if (now.TotalSeconds64() - tempsec2 >= 1800 && SDInserted) {
      SDDateTime(now);
      dataFile.print("\t");
      dataFile.print(moistureP);
      dataFile.print("\t");
      dataFile.print(humidity);
      dataFile.print("\t");
      dataFile.println(temp);
      dataFile.flush();
      tempsec2 = now.TotalSeconds64();
    }

    tempsec1 = now.TotalSeconds64();
  }

  if (micros() < last_micros) last_micros = micros();

}

void lcdPrint(int moistureP, float temp, float humidity) {
  lcd.createChar(0, soilmSymb);
  lcd.createChar(1, humiditySymb);
  lcd.createChar(2, tempSymb);
  lcd.createChar(3, SDOn);
  lcd.createChar(4, SDOff);

  lcd.setCursor(15, 0);
  if (SDInserted) lcd.write(3);
  else lcd.write(4);

  lcd.setCursor(0, 1);
  lcd.write(0);
  lcd.setCursor(1, 1);
  lcd.print("=");
  lcd.setCursor(2, 1);
  lcd.print(moistureP);
  lcd.print("%");
  lcd.print(" ");

  lcd.setCursor(6, 1);
  lcd.write(1);
  lcd.setCursor(7, 1);
  lcd.print("=");
  lcd.setCursor(8, 1);
  lcd.print(humidity, 0);
  lcd.print("%");
  lcd.print(" ");

  lcd.setCursor(12, 1);
  lcd.write(2);
  lcd.setCursor(13, 1);
  lcd.print("=");
  lcd.setCursor(14, 1);
  lcd.print(temp, 0);
  lcd.print(" ");
}

void LCDDateTime(const RtcDateTime& dt) {
  char datestring[20];
  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u %02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Hour(),
             dt.Minute());
  lcd.setCursor(0, 0);
  lcd.print(datestring);
}

void SDDateTime(const RtcDateTime& dt) {
  char datestring[20];
  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute());
  dataFile.print(datestring);
}

void checkRTC() {
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (!Rtc.IsDateTimeValid()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Updating RTC...");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting RTC");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();

  if (now < compiled) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Updating RTC....");
    Rtc.SetDateTime(compiled);
  }
  delay(1000);
  lcd.clear();
}

void pin3_pressed() {
  if ((micros() - last_micros) >= 100000) {
    backlightON = !backlightON;
    last_micros = micros();
  }
}
