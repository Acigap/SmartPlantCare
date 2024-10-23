//This sketch is an SmartPlantCare
#define CUSTOM_TIMEZONE "CST-8" China time zone
#include <TFT_eSPI.h>  // สำหรับหน้าจอ T-display-s3
#include "hothead.h"
#include "watering.h"
#include <HTTPClient.h>
#include <WiFi.h>+
#include "Free_Fonts.h"     //free fonts must be included in the folder and quotes
#include "ControlSDCard.h"  // สำหรับจัดการเรื่อง SD Card และไฟล์
#include "BlynkData.h"
#include "ServerSD.h"
#include "smart_irrigation.h"
#include <ArduinoOTA.h>
#include "Wire.h"
#include "SHT31.h"

const char *hostSDServer = "esp32sd";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;  // GMT +7
const int daylightOffset_sec = 0;
const char *startPlanTimeFilename = "/startPlanTime.txt";

#define RELAY_PIN 1          // พินของ relay
#define SOIL_MOISTURE_PIN 2  // ขาอนาล็อกที่ใช้ในการวัดความชื้นในดิน
#define BUTTOPN_PIN 14       // พินของ ปุ่มล่าง ของจอด้านหน้า

// I2C สำหรับวัดความชื้น SHT31
#define SHT31_ADDRESS   0x44
#define SDA_PIN 21  // ระบุพิน SDA เขียว
#define SCL_PIN 17  // ระบุพิน SCL เหลือง
SHT31 sht;
int sensorMin = 0;
int sensorMax = 0;
int soilMoisture = 0;

// เก็บ State กันการ set ค่าซ้ำ
bool buttonState = 0;
int switchPumState = 0;
bool waterPumpState = 0;
bool checkSoilStat = 0;

// ค่าใหม่สำหรับการควบคุมปั้มน้ำ
unsigned long pumpStartTime = 0; // เวลาที่เริ่มเปิดปั้มน้ำ
unsigned long pumpRunTime = 2 * 60 * 60 * 1000; // 2 ชั่วโมง
unsigned long pumpRestTime = 15 * 60 * 1000; // 15 นาที
bool isPumpCoolingDown = false; // ตัวแปรบันทึกสถานะการพักปั้มน้ำ

int dryValue = 4095;        // ค่าอนาล็อกเมื่อดินแห้ง (เซนเซอร์ไม่อยู่ในน้ำ)
int wetValue = 2400;        // ค่าอนาล็อกเมื่อดินเปียกเต็มที่ (เซนเซอร์อยู่ในน้ำ)
int displayCountDown = 30;  // สำหรับนับถอยหลังเพื่อปิดหน้าจอ
int buttonCountDown = 0;    // hold button count
String lastWatering = "";   // เวลารดน้ำล่าสุด

VeggieType veggie = KALE;  // เลือกชนิดของผัก (คะน้า)

static unsigned long lastLogTime = 0;      // loop check log time
static unsigned long lastDisplayTime = 0;  // loop check display time
static unsigned long lastReadDataTime = 0;  // loop check read data time
time_t plantingTime = 1680000000;          // เวลาในรูปแบบ epoch เมื่อเริ่มปลูก

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
#define PIN_POWER_ON 15  //power enable pin for LCD and use battery
#define PIN_LCD_BL 38    // BackLight enable pin (see Dimming.txt)
String jsonBuffer;
struct tm timeinfo;


int calculateDaysPlanted() {
  time_t now = time(NULL);
  double secondsDiff = difftime(now, plantingTime);
  int daysPlanted = secondsDiff / (60 * 60 * 24);  // แปลงเป็นวัน
  return daysPlanted;
}

int readSoilMoisture() {
  int soilMoisturePercent = 0;
  uint16_t stat = sht.readStatus();
  // Serial.print(stat, HEX);

  if (stat != 0xFFFF) { // check SHT31 senser.
    sht.read(); 
    // Serial.print("\t");
    // Serial.print(sht.getTemperature(), 1);
    // Serial.print("\t");
    // Serial.println(sht.getHumidity(), 1);
    soilMoisturePercent = sht.getHumidity();
    virtualWriteV4(0);
    delay(150);
  } else { // if SHT31 not connect, So get soilMoisture from SOIL_MOISTURE_PIN
    //Serial.println("SHT31 error.");
    float soilMoisture = 0;
    int total = 0;
    int readings = 5;  // จำนวนครั้งที่ต้องการอ่านค่า
    // อ่านค่าจาก SOIL_MOISTURE_PIN จำนวน 3 ครั้งแล้วรวมค่า
    for (int i = 0; i < readings; i++) {
      total += analogRead(SOIL_MOISTURE_PIN);  // อ่านค่า SOIL_MOISTURE_PIN
      delay(30);                               // หน่วงเวลาเล็กน้อย (30 ms) ระหว่างการอ่านแต่ละครั้ง
    }
    // หาค่าเฉลี่ย
    soilMoisture = total / readings;
    // Serial.println("soilMoistureA0: ");
    // Serial.print(average);
    virtualWriteV4(soilMoisture);
    soilMoisturePercent = map(soilMoisture, wetValue, dryValue, 100, 0);
  }
 

  // ตรวจสอบว่าค่าความชื้นอยู่ในช่วงที่ถูกต้อง
  if (soilMoisturePercent >= 100) {
    soilMoisturePercent = 100;
  } else if (soilMoisturePercent <= 0) {
    soilMoisturePercent = 0;
  }
  return soilMoisturePercent;
}

void logSDd(const char *action, const char *value) {
  String data = getCurrentDateTime() + String(",") + String(action) + String(",") + String(value) + String("\n");
  appendFile(csvFilename, data.c_str());
}

void controlWaterPump(bool state, int moisture, bool isManual) {
  if (waterPumpState == state) {
    return;
  }
  logsSoilMoisture(moisture);
  waterPumpState = state;
  displayCountDown = 30;
  char *action = "ControlPump";
  if (isManual) {
    action = "ManualPump";
  } else {
    virtualWriteV0(state);
  }

  if (state) {
    lastWatering = getCurrentDateTime();
    digitalWrite(RELAY_PIN, LOW);  // เปิดปั้มน้ำ
    logSDd(action, "Start");
    pumpStartTime = millis(); // บันทึกเวลาที่เริ่มเปิดปั้มน้ำ
    isPumpCoolingDown = false; // รีเซ็ตสถานะการพัก
    Serial.println("**Open Pump**");
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // ปิดปั้มน้ำ
    logSDd(action, "Stop");
    Serial.println("**Close Pump**");
  }

  lastDisplayTime = 0; // Show info now.
  // Serial.println(String(action) + "" + String(state));
}

void displayInfo(int moisture, int days, String strLastWatering) {
  tft.fillScreen(TFT_BLACK);
  if(waterPumpState) {
    tft.pushImage(165, 10, 155, 170, watering);
  } else {
    tft.pushImage(165, 10, 155, 170, hothead);
  }

  int threshold, stop;
  getMoistureRange(veggie, threshold, stop, sensorMin, sensorMax);

  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSS9);
  tft.setCursor(165, 18);
  tft.println(getCurrentDateTime());
  tft.setCursor(0, 18);
  tft.println("Plant Date: " + String(days));
  tft.println("Veggie: " + String(veggieToString(veggie)));
  tft.println("Water: " + String(threshold) + " Stop: " + String(stop));
  tft.println("Last Watering: ");
  tft.println(strLastWatering);
  tft.println("Soil Moisture(%): " + String(moisture));
  tft.println("Web: http://" + String(hostSDServer) + ".local");
  if(isConnected() == false) {
    tft.setTextColor(TFT_RED);
  } else {
    tft.setTextColor(TFT_GREEN);
  }
  tft.setFreeFont(FSB9);
  tft.setCursor(270, 160);
  tft.println("Blynk");
}

void checkSoilMoisture(int moisture) {
  virtualWriteV2(moisture);
  // ตรวจสอบการควบคุมปั๊มน้ำ
  bool isPumpOn = checkPumpControl(veggie, moisture, sensorMin, sensorMax);
  if (checkSoilStat == isPumpOn) {
    return;
  }
  checkSoilStat = isPumpOn;
  controlWaterPump(isPumpOn, moisture, false);
}

// ฟังก์ชันสำหรับดึงและแสดงวันที่และเวลาปัจจุบันในรูปแบบ yyyy-mm-dd HH:mm
String getCurrentDateTime() {
  // ดึงเวลาปัจจุบันในรูปแบบ epoch time
  time_t now = time(NULL);

  // แปลงเป็นโครงสร้างเวลามนุษย์ (local time)
  struct tm *timeinfo = localtime(&now);

  // ดึงข้อมูลปี เดือน วัน ชั่วโมง และนาที
  int year = timeinfo->tm_year + 1900;  // ปี (tm_year นับจาก 1900)
  int month = timeinfo->tm_mon + 1;     // เดือน (tm_mon เริ่มจาก 0 = มกราคม)
  int day = timeinfo->tm_mday;          // วันของเดือน
  int hour = timeinfo->tm_hour;         // ชั่วโมง
  int minute = timeinfo->tm_min;        // นาที

  // แปลงวันที่และเวลาเป็น String ในรูปแบบ yyyy-mm-dd HH:mm
  char dateTimeBuffer[20];
  snprintf(dateTimeBuffer, sizeof(dateTimeBuffer), "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return String(dateTimeBuffer); 
}

void initWiFi() {
  Serial.println("    Start func init WiFi");
  tft.setFreeFont(FSS9);
  tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
  tft.setTextColor(TFT_RED);
  tft.setCursor(30, 50);
  tft.println("init WiFi");
  Serial.println("init WiFi");
  delay(500);
  WiFi.mode(WIFI_STA);

  connectToWiFi();

  int attempt = 0;
  const int max_attempts = 20;

  // ลูปพยายามเชื่อมต่อ WiFi
  while (WiFi.status() != WL_CONNECTED && attempt < max_attempts) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WL  CONNECTED in initWiFi");
    tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(30, 50);
    tft.println("connected");
    tft.println("LocalIP: " + WiFi.localIP());
    delay(1000);
  } else {
    Serial.println("WL not CONNECTED in initWiFi");
    tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
    tft.setTextColor(TFT_RED);
    tft.setCursor(30, 50);
    tft.println("No WiFi");
  }
  delay(500);
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

// ฟังก์ชันที่ทำงานเมื่อข้อมูลมีการเปลี่ยนแปลง
void onStartAPMode(String ip, String ap, String pwd) {
  tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(30, 50);
  tft.println("Start AP Mode");
  tft.println("AP: " + ap);
  tft.println("PWD: " + pwd);
  tft.println("Server IP: " + ip);
}

void logsSoilMoisture(int soilMoisture) {
  char buf[4];
  itoa(soilMoisture, buf, 10);
  logSDd("SoilMoisture", buf);
}

void createLogFile() {
  String str = readFileToString(csvFilename);
  if (str.isEmpty()) {
    Serial.println("writeFile: " + String(csvFilename));
    writeFile(csvFilename, "time, action, value\n");
  }
}

void checkPumpCoolingDown() {
  unsigned long currentMillis = millis();
  if (isPumpCoolingDown) {
    if (currentMillis - pumpStartTime >= pumpRestTime) {
      isPumpCoolingDown = false; // หมดเวลา พักปั้มน้ำ
      if (waterPumpState) {
        digitalWrite(RELAY_PIN, LOW); // เปิดปั้มน้ำใหม่
        pumpStartTime = millis(); // บันทึกเวลาที่เริ่มเปิดปั้มน้ำ
      }
    }
  } else if (waterPumpState) {
    if (currentMillis - pumpStartTime >= pumpRunTime) {
      digitalWrite(RELAY_PIN, HIGH);// ปิดปั้มน้ำเมื่อถึงเวลา
      pumpStartTime = currentMillis; // เริ่มพัก
      isPumpCoolingDown = true; // กำลังพัก
    }
  }
}

void setupSHT31() {
  Serial.println("setupSHT31");
  Wire.begin(SDA_PIN, SCL_PIN);
  sht.begin();
  Wire.setClock(100000);

  uint16_t stat = sht.readStatus();
  Serial.print(stat, HEX);
  Serial.println();
}

void setUpOTA() {
  Serial.println("setUpOTA");
  // เริ่มต้นการทำงาน OTA
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: หากใช้ SPIFFS ให้ปิด SPIFFS ก่อนที่จะเริ่มการอัปเดต
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.setHostname(hostSDServer);
  ArduinoOTA.begin();
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);  // ตั้งพินสำหรับ relay เป็น output
  pinMode(BUTTOPN_PIN, INPUT);
  digitalWrite(RELAY_PIN, HIGH);  // ปิด relay ตอนเริ่มต้น
  pinMode(PIN_POWER_ON, OUTPUT);  //enables the LCD and to run on battery
  pinMode(PIN_LCD_BL, OUTPUT);    //triggers the LCD backlight
  digitalWrite(PIN_POWER_ON, HIGH);
  digitalWrite(PIN_LCD_BL, HIGH);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.begin(115200);  // be sure to set USB CDC On Boot: "Enabled"

  setUpOTA();
  setupSDCard();

  tft.fillRect(0, 165, 130, 60, TFT_CYAN);  //horiz, vert
  tft.setFreeFont(FSS9);
  tft.setTextColor(TFT_RED);
  tft.setCursor(30, 50);
  tft.print("in setup");
  tft.init();
  Serial.println("          in setup");
  Serial.println("  ");
  tft.setRotation(3);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);  //horiz / vert<> position/dimension
  tft.pushImage(165, 0, 155, 170, hothead);
  tft.setTextColor(TFT_GREEN);
  tft.setFreeFont(FSB12);
  tft.setCursor(5, 25);
  tft.println("SmartPlantCare");

  initWiFi();                                  // ตั้งค่า wifi
  setupBlynk();                                // ตั้งค่า Blynk lib
  setupServerSD(hostSDServer, onStartAPMode);  // ตั้งค่า web server

  Serial.println("Setup log file.");
  // log
  createLogFile();
  logSDd("Esp32Start", "initOK");
  String startStr = readFileToString(startPlanTimeFilename);
  if (startStr.isEmpty()) {
     plantingTime = time(NULL);
  } else {
    plantingTime = startStr.toInt();
    Serial.println("startStr: " + startStr);
    Serial.println(plantingTime);
  }
  
  Serial.println("Read preferences and setup ConfigParameter");
  preferences.begin("VeggieType", false);
  int32_t veggieTypeInt = preferences.getInt("selectedVeggie", 5);
  // ดึงค่าจาก Preferences
  veggie = static_cast<VeggieType>(veggieTypeInt);  // แปลงไปเป็น VeggieType
  preferences.end();
  Serial.println("VeggieType: " + String(veggieTypeInt));

  preferences.begin("ConfigParameter", false);
  wetValue = preferences.getInt("selectedWet", wetValue);  // ค่าเริ่มต้นคือ wetValue
  dryValue = preferences.getInt("selectedDry", dryValue);  // ค่าเริ่มต้นคือ dryValue
  pumpRunTime = preferences.getLong("pumpRunTime", pumpRunTime);  // ค่าเริ่มต้นคือ 0
  pumpRestTime = preferences.getLong("coolingDownTime", pumpRunTime);  // ค่าเริ่มต้นคือ 0
  preferences.end();
  // Serial.println("WetValue: " + String(wetValue));
  // Serial.println("DryValue: " + String(dryValue));
  setupSHT31();
  virtualWriteV0(0); // Send 0 when fist open
  Serial.println("**************** SetUp End ****************");
}

void loop() {
  ArduinoOTA.handle();                    // ตรวจสอบการเชื่อมต่อ OTA
  loopServerSD();
  loopBlynk();

  if (millis() - lastReadDataTime >= 500) {  // 500 ms = 0.5 วินาที
    sensorMin = getSensorMin();
    sensorMax = getSensorMax(); 
    soilMoisture = readSoilMoisture();  // อ่านค่าความชื้นในดิน
    checkSoilMoisture(soilMoisture);        // ตรวจสอบความชื้นในดินและสั่งรดน้ำ
    lastReadDataTime = millis();  
  }
  if (millis() - lastDisplayTime >= 1000) {  // 1000 ms = 1 วินาที
    // แสดงข้อมูลบนหน้าจอ
    int daysPlanted = calculateDaysPlanted();              // คำนวณจำนวนวันที่ปลูกมาแล้ว
    displayInfo(soilMoisture, daysPlanted, lastWatering);  // อัปเดตข้อมูลบนหน้าจอ
    lastDisplayTime = millis();                            // อัปเดตเวลาล่าสุดที่แสดงจอ
    displayCountDown--;
    if (displayCountDown < 0) { displayCountDown = 0; }
  }
  if (millis() - lastLogTime >= 300000) {  // 1000 = 1sec. * 60 * 5  = 300,000 sec. (5 นาที)
    logsSoilMoisture(soilMoisture);
    lastLogTime = millis();  // อัปเดตเวลาล่าสุดที่บันทึกข้อมูล
  }

  // Check switchPum from Blynk server
  int switchPumData = getSwitchPum();
  if (switchPumState != switchPumData) {
    controlWaterPump(switchPumData, soilMoisture, true);
    switchPumState = switchPumData;
  }

  checkPumpCoolingDown();

  buttonState = digitalRead(BUTTOPN_PIN);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    displayCountDown = 30;
    buttonCountDown--;
    if (buttonCountDown < 0) {
      plantingTime = time(NULL);
      char temp[10];
      ltoa(plantingTime, temp, 10);
      writeFile(startPlanTimeFilename, temp);
      buttonCountDown = 10;
      Serial.println(plantingTime);
      Serial.println(temp);

      // เคลียร์ log file โดยการเปลียนชื่อเป็น timestamp แล้วสร้างใหม่
      String filename = "/" + String(temp) + ".csv";
      renameFile(csvFilename, filename.c_str());
      delay(200);
      createLogFile();

      tft.fillScreen(TFT_BLACK);
      tft.pushImage(165, 0, 155, 170, hothead);
      tft.setCursor(10, 50);
      tft.setFreeFont(FSS9);
      tft.setTextColor(TFT_GREEN);
      tft.println("Set start plant finish..");
      delay(3000);
    } else {
      Serial.println(buttonCountDown);
      if (buttonCountDown <= 9) {  // ไม่แสดงค่านับถอนหลัง้ากดไปแค่ 1-2 วินาที
        tft.fillRect(0, 140, 165, 175, TFT_BLACK);  //horiz, vert
        tft.setCursor(0, 155);
        tft.setTextColor(TFT_BLUE);
        tft.println("Set start plant <-- " + String(buttonCountDown));
      }
    }
    delay(500);
  } else {
    buttonCountDown = 10;
  }

  if (displayCountDown <= 0) {
    // turn LED off:
    digitalWrite(PIN_LCD_BL, LOW);
  } else {
    // turn LED on:
    digitalWrite(PIN_LCD_BL, HIGH);
  }
  delay(100);
}