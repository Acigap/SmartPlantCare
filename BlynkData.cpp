/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

 /* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "xxxxxx"
#define BLYNK_TEMPLATE_NAME         "xxxxxx"
#define BLYNK_AUTH_TOKEN            "xxxxxx"

#include <WiFi.h>
#include <WiFiClient.h>
#include "BlynkData.h"
#include <BlynkSimpleEsp32.h>
#include <Preferences.h>  // เพิ่มการเรียกใช้ Preferences library

const char* host = "blynk.cloud";
int port = 80;
static int switchPum = 0;
int _sensorMin = 0;
int _sensorMax = 0;
unsigned long _connectedTime = 0;

BLYNK_CONNECTED() {
    Blynk.syncAll();
}

int getSensorMin() {
  return _sensorMin;
}

int getSensorMax() {
  return _sensorMax;
}

int getSwitchPum() {
  return switchPum;
}

BLYNK_WRITE(V0)
{   
  switchPum = (param.asInt()); // Get value as integer
  Serial.println("switchPum:");
  Serial.println(switchPum);

  // The param can contain multiple values, in such case:
  // int x = param[0].asInt();
  // int y = param[1].asInt();
}

BLYNK_WRITE(V5)
{   
  _sensorMin = (param.asInt()); // Get value as integer
  Preferences preferences;
  preferences.begin("ConfigParameter", false);
  preferences.putInt("sensorMin", _sensorMin);   
  preferences.end();
  Serial.println("sensorMin:");
  Serial.println(_sensorMin);
}

BLYNK_WRITE(V6)
{   
  _sensorMax = (param.asInt()); // Get value as integer
  Preferences preferences;
  preferences.begin("ConfigParameter", false);
  preferences.putInt("sensorMax", _sensorMax);  
  preferences.end();
  Serial.println("sensorMax:");
  Serial.println(_sensorMax);
}

void virtualWriteV0(int value) {
  switchPum = value;
  Blynk.virtualWrite(V0,value);
}

void virtualWriteV2(int value) {
  Blynk.virtualWrite(V2,value);
}

void virtualWriteV4(int value) {
  Blynk.virtualWrite(V4,value);
}

bool isConnected() {
  return Blynk.connected();
}

void setupBlynk()
{
  Serial.println("setupBlynk");
  // อ่านค่า BlynkConfig จาก Preferences
  Preferences preferences;
  preferences.begin("BlynkConfig", false);     
  String blynkAuthToken = preferences.getString("authToken", String(BLYNK_AUTH_TOKEN));  
  String blynkTemplateID = preferences.getString("templateID", String(BLYNK_TEMPLATE_ID));  
  String blynkTemplateName = preferences.getString("templateName", String(BLYNK_TEMPLATE_NAME));  
  preferences.end();
  delay(200);
  preferences.begin("ConfigParameter", false);
  _sensorMin = preferences.getInt("sensorMin", _sensorMin);  // ค่าเริ่มต้นคือ sensorMin
  _sensorMax = preferences.getInt("sensorMax", _sensorMax);  // ค่าเริ่มต้นคือ sensorMax
  preferences.end();


  if (WiFi.status() == WL_CONNECTED) {
    // เริ่มต้น Blynk
    Blynk.config(blynkAuthToken.c_str());
    Serial.println("Connecting Blynk...");
    if(Blynk.connect(20000) == false) { // time out 20 sec.
      Serial.println("Fail to onnect Blynk server!!");
      _connectedTime = 0;
    } else {
      _connectedTime =  millis();
      Serial.println("Connected to Blynk server");
    }
  }
}

void loopBlynk()
{
  if(Blynk.connected() == false) {
    if(_connectedTime != 0) { // เคย เชื่อมมามาแล้วแต่หลุด
      ESP.restart();
    } else if(WiFi.status() == WL_CONNECTED) { // ยังไม่เคยต่อได้แต่ ต่อ wifi ได้แล้ว 
      ESP.restart();
    }
  }
  Blynk.run();
}
