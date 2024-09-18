//This sketch is an example of reading time from NTP
#define CUSTOM_TIMEZONE "CST-8" China time zone
#include <TFT_eSPI.h>    // สำหรับหน้าจอ T-display-s3
#include "hothead.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include "Free_Fonts.h"  //free fonts must be included in the folder and quotes
#include "ControlSDCard.h"  // สำหรับจัดการเรื่อง SD Card และไฟล์
#include "BlynkData.h"

const char *ssid = "iStyleM";
const char *password = "015321418gG";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200;  // GMT +7
const int daylightOffset_sec = 0;

#define RELAY_PIN 1           // พินของ relay
#define SOIL_MOISTURE_PIN 2  // ขาอนาล็อกที่ใช้ในการวัดความชื้นในดิน
#define BUTTOPN_PIN 14           // พินของ ปุ่มล่าง ของจอด้านหน้า

bool buttonState = 0;  // อ่านค่าปุ่มกด
int dryValue = 4095;  // ค่าอนาล็อกเมื่อดินแห้ง (เซนเซอร์ไม่อยู่ในน้ำ)
int wetValue = 2300;     // ค่าอนาล็อกเมื่อดินเปียกเต็มที่ (เซนเซอร์อยู่ในน้ำ) 
int soilMoisture = 0;  // ค่าความชื้อนนดินที่อ่านได้
int oldMoisture = 0;   // ค่าก่อน update
String lastWatering = ""; // เวลารดน้ำล่าสุด

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
  //Serial.print("soilMoistureValue:");
  //Serial.println(soilMoistureValue);
  // แปลงค่าอนาล็อกเป็นเปอร์เซ็นต์
  int soilMoisturePercent = map(soilMoistureValue, wetValue, dryValue, 100, 0);

  // ตรวจสอบว่าค่าความชื้นอยู่ในช่วงที่ถูกต้อง
  if (soilMoisturePercent > 100) {
    soilMoisturePercent = 100;
  } else if (soilMoisturePercent < 0) {
    soilMoisturePercent = 0;
  }
 
  virtualWriteV2(soilMoisturePercent);
  return soilMoisturePercent;
}

void controlWaterPump(bool state) {
  if (state) {
    lastWatering = getCurrentDateTime();
    digitalWrite(RELAY_PIN, LOW);  // เปิดปั้มน้ำ
    virtualWriteV0(1);
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // ปิดปั้มน้ำ
    virtualWriteV0(0);
  }
}

void displayInfo(int moisture, int days, String strLastWatering) {
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(165, 0, 155, 170, hothead);
  tft.setCursor(0, 20);
  tft.setFreeFont(FSS9);
  tft.setTextColor(TFT_WHITE);

  tft.println("Current Time:");
  tft.println(getCurrentDateTime());
  tft.println("Last Watering: ");
   tft.println(strLastWatering);
  tft.println("Soil Moisture: " + String(moisture));
  tft.println("Days: " + String(days));
}

void checkSoilMoisture(int moisture) {
  if (oldMoisture > 40 && moisture < 40) {  // ค่าความชื้นต่ำ
    controlWaterPump(true);
    //logData(moisture, "Auto Watering");
  } else if (oldMoisture <40 && moisture > 40) {
    controlWaterPump(false);
  }

  oldMoisture = moisture;
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

  writeFile("/hello.txt", "Hello ");
  appendFile("/hello.txt", "World!\n");
  readFile("/hello.txt");

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

  initWiFi();
  setupBlynk();
}

void loop() {
  if (millis() % 500 == 0 ) {
    soilMoisture = readSoilMoisture();  // อ่านค่าความชื้นในดิน
    checkSoilMoisture(soilMoisture);  // ตรวจสอบความชื้นในดินและสั่งรดน้ำ

    if(getSwitchPum() > 0) {
      digitalWrite(RELAY_PIN, LOW);  // เปิดปั้มน้ำ  // เปิดปั้มน้ำ
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // เปิดปั้มน้ำ  // ปิดปั้มน้ำ
    }
  }

  buttonState = digitalRead(BUTTOPN_PIN);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    digitalWrite(PIN_LCD_BL, HIGH);
  } else {
    // turn LED off:
    digitalWrite(PIN_LCD_BL, LOW);
  }

  if (millis() - lastDisplayTime >= 3000) {  // 3000 ms = 1 วินาที
    // แสดงข้อมูลบนหน้าจอ
    int daysPlanted = calculateDaysPlanted();         // คำนวณจำนวนวันที่ปลูกมาแล้ว
    displayInfo(soilMoisture, daysPlanted, lastWatering);  // อัปเดตข้อมูลบนหน้าจอ
    lastDisplayTime = millis();                 // อัปเดตเวลาล่าสุดที่แสดงจอ
  }

  loopBlynk();
}