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

BLYNK_CONNECTED() {
    Blynk.syncAll();
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

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(blynkAuthToken.c_str());  
    // เริ่มต้น Blynk
    //Blynk.begin(blynkAuthToken.c_str(), blynkTemplateID.c_str(), blynkTemplateName.c_str());
    Blynk.connect(3000);  // time out 10 sec.
    while (Blynk.connect() == false) {
      // Wait until connected
      Serial.println(".");
    }
    Serial.println("Connected to Blynk server");
  }
}

void loopBlynk()
{
  Blynk.run();
}
