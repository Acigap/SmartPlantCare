//This sketch is an SmartPlantCare 
#define CUSTOM_TIMEZONE "CST-8" China time zone
#include <TFT_eSPI.h>    // สำหรับหน้าจอ T-display-s3
#include "hothead.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include "Free_Fonts.h"  //free fonts must be included in the folder and quotes
#include "ControlSDCard.h"  // สำหรับจัดการเรื่อง SD Card และไฟล์
#include "BlynkData.h"
#include "ServerSD.h"
#include "smart_irrigation.h"

const char *ssid = "iStyleM";
const char *password = "015321418gG";
const char* hostSDServer = "esp32sd";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;  // GMT +7
const int daylightOffset_sec = 0;
const char *startPlanTimeFilename = "/startPlanTime.txt";

#define RELAY_PIN 1           // พินของ relay
#define SOIL_MOISTURE_PIN 2  // ขาอนาล็อกที่ใช้ในการวัดความชื้นในดิน
#define BUTTOPN_PIN 14           // พินของ ปุ่มล่าง ของจอด้านหน้า

bool buttonState = 0;  // อ่านค่าปุ่มกด
int switchPumState = 0;
bool waterPumpState = 0;
int dryValue = 4000;  // ค่าอนาล็อกเมื่อดินแห้ง (เซนเซอร์ไม่อยู่ในน้ำ)
int wetValue = 0;     // ค่าอนาล็อกเมื่อดินเปียกเต็มที่ (เซนเซอร์อยู่ในน้ำ) 
int soilMoisture = 0;  // ค่าความชื้อนนดินที่อ่านได้
int displayCountDown = 30; // สำหรับนับถอยหลังเพื่อปิดหน้าจอ
int buttonCountDown = 0; // hold button count
String lastWatering = ""; // เวลารดน้ำล่าสุด  

VeggieType veggie = CHILI;  // เลือกชนิดของผัก (พริก)

static unsigned long lastLogTime = 0;  // loop check log time
static unsigned long lastDisplayTime = 0; // loop check display time
time_t plantingTime = 1680000000;  // เวลาในรูปแบบ epoch เมื่อเริ่มปลูก TODO: ต้องทำตัวเก็บเวลาเริ่มต้น

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
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);  // อ่านค่าจากเซนเซอร์
  // Serial.print("soilMoisture: ");
  // Serial.println(soilMoistureValue);
  // แปลงค่าอนาล็อกเป็นเปอร์เซ็นต์
  int soilMoisturePercent = map(soilMoistureValue, wetValue, dryValue, 100, 0);

  // ตรวจสอบว่าค่าความชื้นอยู่ในช่วงที่ถูกต้อง
  if (soilMoisturePercent > 100) {
    soilMoisturePercent = 100;
  } else if (soilMoisturePercent < 0) {
    soilMoisturePercent = 0;
  }
  return soilMoisturePercent;
}

void logSDd(const char *action, const char *value) {
  String data = getCurrentDateTime() +  String(",") + String(action) +  String(",") + String(value) +  String("\n");
  appendFile(csvFilename, data.c_str());
}

void controlWaterPump(bool state) {
  if(waterPumpState == state) {
    return;
  }
  logsSoilMoisture(readSoilMoisture());
  waterPumpState = state;
  displayCountDown = 30;
  if (state) {
    lastWatering = getCurrentDateTime();
    digitalWrite(RELAY_PIN, LOW);  // เปิดปั้มน้ำ
    virtualWriteV0(1);
    logSDd("WaterPumpStateChange", "Start");
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // ปิดปั้มน้ำ
    virtualWriteV0(0);
    logSDd("WaterPumpStateChange", "Stop");
  }
}

void displayInfo(int moisture, int days, String strLastWatering) {
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(165, 10, 155, 170, hothead);

  int threshold, stop;
  getMoistureRange(veggie, threshold, stop);

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
  tft.println("Soil Moisture(%): " + String(moisture) + "\n");
}

void checkSoilMoisture(int moisture) {
  // ตรวจสอบการควบคุมปั๊มน้ำ
  bool isPumpOn = checkPumpControl(veggie, moisture);
  controlWaterPump(isPumpOn);
  virtualWriteV2(moisture);
}

// ฟังก์ชันสำหรับดึงและแสดงวันที่และเวลาปัจจุบันในรูปแบบ yyyy-mm-dd HH:mm
String getCurrentDateTime() {
  // ดึงเวลาปัจจุบันในรูปแบบ epoch time
  time_t now = time(NULL);

  // struct tm timeinfo;
  // if(!getLocalTime(&timeinfo)){
  //   Serial.println("Failed to obtain time");
  // }

  // แปลงเป็นโครงสร้างเวลามนุษย์ (local time)
  struct tm* timeinfo = localtime(&now);

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
  return String(dateTimeBuffer);  // คืนค่าเป็น String
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WL  CONNECTED in initWiFi");
    tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(30, 50);
    tft.println("connected");
    delay(1000);
  } else {
    Serial.println("WL not CONNECTED in initWiFi");
    tft.fillRect(10, 32, 130, 25, TFT_BLACK);  //horiz, vert
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(30, 50);
    tft.println("No WiFi");
  }
  delay(500);
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void logsSoilMoisture(int soilMoisture) {
  char buf[4];
  itoa(soilMoisture, buf, 10);
  logSDd("SoilMoisture",buf);
}

void createLogFile() {
  String str = readFileToString(csvFilename);
  if (str.isEmpty()) {
    Serial.println("writeFile: " + String(csvFilename));
    writeFile(csvFilename,"time, action, value\n");
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);    // ตั้งพินสำหรับ relay เป็น output
  pinMode(BUTTOPN_PIN, INPUT);
  digitalWrite(RELAY_PIN, HIGH);  // ปิด relay ตอนเริ่มต้น
  pinMode(PIN_POWER_ON, OUTPUT);  //enables the LCD and to run on battery
  pinMode(PIN_LCD_BL, OUTPUT);    //triggers the LCD backlight
  digitalWrite(PIN_POWER_ON, HIGH);
  digitalWrite(PIN_LCD_BL, HIGH);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.begin(115200);  // be sure to set USB CDC On Boot: "Enabled"

  setupSDCard();

  WiFi.mode(WIFI_STA);
  tft.fillRect(0, 165, 130, 60, TFT_CYAN);  //horiz, vert
  tft.setFreeFont(FSS9);
  tft.setTextColor(TFT_RED);
  tft.setCursor(30, 50);
  tft.print("in setup");
  tft.init();
  WiFi.begin(ssid, password);
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

  initWiFi();    // ตั้งค่า wifi
  setupBlynk();   // ตั้งค่า Blynk lib
  setupServerSD(hostSDServer);  // ตั้งค่า web server

  // log
  createLogFile();
  logSDd("Start", "init ok.");
  //readFile(csvFilename);
 // readFile(startPlanTimeFilename);
  String startStr = readFileToString(startPlanTimeFilename);
  plantingTime = startStr.toInt(); 
  Serial.println("startStr: " + startStr);
  Serial.println(plantingTime);
}

void loop() {
  soilMoisture = readSoilMoisture();  // อ่านค่าความชื้นในดิน
  checkSoilMoisture(soilMoisture);  // ตรวจสอบความชื้นในดินและสั่งรดน้ำ
  
  if (millis() - lastDisplayTime >= 3000) {  // 3000 ms = 1 วินาที
    // แสดงข้อมูลบนหน้าจอ
    int daysPlanted = calculateDaysPlanted();         // คำนวณจำนวนวันที่ปลูกมาแล้ว
    displayInfo(soilMoisture, daysPlanted, lastWatering);  // อัปเดตข้อมูลบนหน้าจอ
    lastDisplayTime = millis();                 // อัปเดตเวลาล่าสุดที่แสดงจอ
    displayCountDown--;
    if(displayCountDown <0) {displayCountDown = 0;}
  }
  if (millis() - lastLogTime >= 180000) {  // 3000 = 1sec. * 60  = 18000sec. (1 นาที)
    logsSoilMoisture(soilMoisture);
    lastLogTime = millis();                 // อัปเดตเวลาล่าสุดที่บันทึกข้อมูล
  }

  // Check switchPum from Blynk server
  int switchPumData = getSwitchPum();
  if(switchPumState != switchPumData) {
    if(switchPumData > 0) {
      digitalWrite(RELAY_PIN, LOW);  // เปิดปั้มน้ำ  // เปิดปั้มน้ำ
      logSDd("controlWaterPump", "Start");
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // เปิดปั้มน้ำ  // ปิดปั้มน้ำ
      logSDd("controlWaterPump", "Stop");
    }
    switchPumState = switchPumData;
  }

  buttonState = digitalRead(BUTTOPN_PIN);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    displayCountDown = 30;
    buttonCountDown--;
    if(buttonCountDown < 0) {
      plantingTime = time(NULL);
      char temp[10];
      ltoa(plantingTime,temp,10);
      writeFile(startPlanTimeFilename, temp);
      buttonCountDown = 33;
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
      tft.fillRect(5, 145, 165, 175, TFT_BLACK);  //horiz, vert
      tft.setCursor(5, 160);
      if(buttonCountDown <= 30) {// ไม่แสดงค่านับถอนหลัง้ากดไปแค่ 1-2 วินาที
        tft.println("Set start plant <-- " + String(buttonCountDown));
      }
    }
  } else {
    buttonCountDown = 33;
  }

  if(displayCountDown <= 0) {
     // turn LED off:
    digitalWrite(PIN_LCD_BL, LOW);
  } else {
     // turn LED on:
    digitalWrite(PIN_LCD_BL, HIGH);
  }

  loopBlynk();
  loopServerSD();
}