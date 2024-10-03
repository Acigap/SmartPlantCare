#include <ESPmDNS.h>  // สำหรับ จัดการ host name
#include "ServerSD.h"
#include <WiFi.h>
#include <WebServer.h>  // สำหรับ Web Server
#include "FS.h"         // สำหรับจัดการไฟล์
#include "SD_MMC.h"     // สำหรับจัดการ SD card

const char *csvFilename = "/log.csv";
const char *hostLocal;
const char *apName = "MyPlant";
const char *apPWD = "12345678";

WebServer server(80);
Preferences preferences;

// ตัวแปร global เพื่อเก็บ callback ที่รับเข้ามา
void (*storedCallback)(String, String, String) = nullptr;

void setupServerSD(const char *host, void (*callback)(String, String, String)) {
  Serial.setDebugOutput(false);
  Serial.print("\n");
  hostLocal = host;
  // เก็บ callback ที่รับเข้ามาไว้ในตัวแปร global
  storedCallback = callback;

  // ถ้าเชื่อมต่อสำเร็จ
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    startSDServer();
    startHostName(host);
  } else {
    // ถ้าเชื่อมต่อไม่สำเร็จ
    Serial.println("\nFailed to connect, starting AP mode...");
    startAPMode();
    // รอให้ผู้ใช้ตั้งค่า WiFi ใหม่
    while (WiFi.status() != WL_CONNECTED) {
      server.handleClient();  // รอให้ผู้ใช้กรอก SSID และ Password ใหม่
      delay(100);
    }
    startHostName(host);
  }
}

void startHostName(const char *host) {
  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }
}

void startSDServer() {
  server.on("/download", HTTP_GET, []() {
    File file = SD_MMC.open(csvFilename);
    if (!file) {
      Serial.println("Failed to open file for reading");
      server.send(500, "text/plain", "Failed to open file");
      return;
    }

    if (server.streamFile(file, "text/csv") != file.size()) {
      Serial.println("Sent less data than expected!");
    }
  });

  startWifiServer();
  server.begin();
}

// ฟังก์ชันเริ่มโหมด AP
void startAPMode() {
  WiFi.softAP(apName, apPWD);
  Serial.println("Access Point started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  startWifiServer();
  server.begin();
  Serial.println("Web server started");
  // ตรวจสอบว่ามี callback หรือไม่
  if (storedCallback != nullptr) {
    storedCallback(WiFi.softAPIP().toString(), String(apName), String(apPWD));
  }
}

// ฟังก์ชันในการอ่านข้อมูลจากไฟล์ CSV
String readCSV() {
  File file = SD_MMC.open(csvFilename);
  if (!file) {
    return "";
  }
  
  String data = "";
  while (file.available()) {
    data += file.readStringUntil('\n') + "\n";
  }
  
  file.close();
  return data;
}

void startWifiServer() {
  server.on("/", []() {
    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; display: flex; justify-content: center; align-items: flex-start; height: 100vh; }";
    html += ".container { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); width: 100%; max-width: 400px; text-align: center; }";
    html += "h1 { color: #5a5a5a; }";
    html += "h2 { color: #4CAF50; }";
    html += "ul { list-style-type: none; padding: 0; }";
    html += "li { margin: 10px 0; }";
    html += "a { text-decoration: none; color: #4CAF50; padding: 10px; border: 1px solid #4CAF50; border-radius: 5px; display: inline-block; transition: background-color 0.3s, color 0.3s; }";
    html += "a:hover { background-color: #4CAF50; color: white; }";
    html += "</style></head><body><div class='container'>";
    html += "<h1>SmartPlantCare</h1>";
    html += "<p>Mode: " + String(WiFi.softAPIP().toString() != "0.0.0.0" ? "AP Mode" : "WiFi Connected") + "</p>";
    html += "<p>SSID: " + String(WiFi.SSID()) == "" ? String(apName) : String(WiFi.SSID()) + "</p>";
    html += "<h2>Select an Option</h2>";
    html += "<ul>";
    html += "<li><a href='/wifiSetup'>Setup WiFi</a></li>";
    html += "<li><a href='/download'>Download CSV Log</a></li>";
    html += "<li><a href='/veggieSelection'>Select Veggie Type</a></li>";
    html += "<li><a href='/configParameter'>Config parameter</a></li>";  // เพิ่มหน้าใหม่สำหรับการตั้งค่า wetValue
    html += "<li><a href='/configBlynk'>Config Blynk Type</a></li>";
    html += "<li><a href='/chart'>Chart</a></li>";
    html += "</ul>";
    html += "</div></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/chart", HTTP_GET, []() {
    File file = SD_MMC.open("/chart.html");
    if (!file) {
      Serial.println("Failed to open file for reading");
      server.send(500, "text/plain", "Failed to open file");
      return;
    }

    if (server.streamFile(file, "text/html") != file.size()) {
      Serial.println("Sent less data than expected!");
    }
  });

  // ส่งข้อมูล CSV ไปยัง client
  server.on("/data", []() {
    String csvData = readCSV();
    server.send(200, "text/plain", csvData);
  });

  server.on("/veggieSelection", []() {
    preferences.begin("VeggieType", false);
    int veggieType = preferences.getInt("selectedVeggie", 5);
    preferences.end();
    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "select { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; }";
    html += "input[type='submit'], input[type='button'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>Select Veggie Type</h1>";
    html += "<form action='/setVeggie' method='POST'>";

    // Select dropdown with default selection based on veggieType
    html += "<select name='veggie'>";
    html += "<option value='0'" + String(veggieType == 0 ? " selected" : "") + ">ผักชี - ปลูก: 30-45 วัน, รดน้ำ: 40%, หยุดรดน้ำ: 60%</option>";
    html += "<option value='1'" + String(veggieType == 1 ? " selected" : "") + ">กะเพรา - ปลูก: 45-60 วัน, รดน้ำ: 35%, หยุดรดน้ำ: 55%</option>";
    html += "<option value='2'" + String(veggieType == 2 ? " selected" : "") + ">โหระพา - ปลูก: 45-60 วัน, รดน้ำ: 40%, หยุดรดน้ำ: 60%</option>";
    html += "<option value='3'" + String(veggieType == 3 ? " selected" : "") + ">ผักกาดขาว - ปลูก: 50-60 วัน, รดน้ำ: 50%, หยุดรดน้ำ: 70%</option>";
    html += "<option value='4'" + String(veggieType == 4 ? " selected" : "") + ">ผักบุ้ง - ปลูก: 25-30 วัน, รดน้ำ: 40%, หยุดรดน้ำ: 60%</option>";
    html += "<option value='5'" + String(veggieType == 5 ? " selected" : "") + ">คะน้า - ปลูก: 40-50 วัน, รดน้ำ: 45%, หยุดรดน้ำ: 65%</option>";
    html += "<option value='6'" + String(veggieType == 6 ? " selected" : "") + ">ต้นหอม - ปลูก: 30-40 วัน, รดน้ำ: 35%, หยุดรดน้ำ: 55%</option>";
    html += "<option value='7'" + String(veggieType == 7 ? " selected" : "") + ">ผักกาดหอม - ปลูก: 40-50 วัน, รดน้ำ: 50%, หยุดรดน้ำ: 70%</option>";
    html += "<option value='8'" + String(veggieType == 8 ? " selected" : "") + ">ผักกาดแก้ว - ปลูก: 55-65 วัน, รดน้ำ: 45%, หยุดรดน้ำ: 65%</option>";
    html += "<option value='9'" + String(veggieType == 9 ? " selected" : "") + ">แตงกวา - ปลูก: 50-60 วัน, รดน้ำ: 50%, หยุดรดน้ำ: 70%</option>";
    html += "<option value='10'" + String(veggieType == 10 ? " selected" : "") + ">พริก - ปลูก: 60-90 วัน, รดน้ำ: 35%, หยุดรดน้ำ: 55%</option>";
    html += "</select><br>";

    html += "<br><input type='submit' value='Select' style='margin-right: 10px;'>";
    html += "<input type='button' value='Cancel' onclick='window.history.back();'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });


  server.on("/wifiSetup", []() {
    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "input[type='text'], input[type='password'] { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; width: 100%; }";
    html += "input[type='submit'], input[type='button'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form action='/save' method='POST'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "<br>Password: <input type='password' name='password'><br>";
    html += "<br><input type='submit' value='Save' style='margin-right: 10px;'>";
    html += "<input type='button' value='Cancel' onclick='window.history.back();'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/configBlynk", []() {
    // อ่านค่า BlynkConfig จาก Preferences
    preferences.begin("BlynkConfig", false);
    String blynkTemplateID = preferences.getString("templateID", "");                    
    String blynkTemplateName = preferences.getString("templateName", "");       
    String blynkAuthToken = preferences.getString("authToken", "");  
    preferences.end();

    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "input[type='text'] { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; width: 100%; }";
    html += "input[type='submit'], input[type='button'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>Config Blynk</h1>";
    html += "<form action='/saveConfigBlynk' method='POST'>";
    html += "BLYNK_TEMPLATE_ID: <input type='text' name='blynkTemplateID' value='" + blynkTemplateID + "'><br>";                // ตั้งค่า default
    html += "<br>BLYNK_TEMPLATE_NAME: <input type='text' name='blynkTemplateName' value='" + blynkTemplateName + "'><br>";      // ตั้งค่า default
    html += "<br>BLYNK_AUTH_TOKEN: <input type='text' name='blynkAuthToken' value='" + blynkAuthToken + "'><br>";               // ตั้งค่า default
    html += "<br><input type='submit' value='Save' style='margin-right: 10px;'>";
    html += "<input type='button' value='Cancel' onclick='window.history.back();'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

   server.on("/configParameter", []() {
    // อ่านค่า wetValue จาก Preferences
    preferences.begin("ConfigParameter", false);
    int wetValue = preferences.getInt("selectedWet", 0);                        // ค่าเริ่มต้นคือ 0
    int dryValue = preferences.getInt("selectedDry", 0);                        // ค่าเริ่มต้นคือ 0
    unsigned long pumpRunTime = preferences.getLong("pumpRunTime", 0);          // ค่าเริ่มต้นคือ 0
    unsigned long coolingDownTime = preferences.getLong("coolingDownTime", 0);  // ค่าเริ่มต้นคือ 0
    preferences.end();
    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "input[type='number'] { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; width: 100%; }";
    html += "input[type='submit'], input[type='button'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>Config parameter</h1>";
    html += "<form action='/saveConfigParameter' method='POST'>";
    html += "Max Wet Value (0-4059): <input type='number' name='wetValue' value='" + String(wetValue) + "'><br>";                          // ตั้งค่า default
    html += "<br>Max Dry Value (0-4059): <input type='number' name='dryValue' value='" + String(dryValue) + "'><br>";                      // ตั้งค่า default
    html += "<br>Max Pump RunTime (Sec.): <input type='number' name='pumpRunTime' value='" + String(pumpRunTime) + "'><br>";               // ตั้งค่า default
    html += "<br>Pump CoolingDown Time (Sec.): <input type='number' name='coolingDownTime' value='" + String(coolingDownTime) + "'><br>";  // ตั้งค่า default
    html += "<br><input type='submit' value='Save' style='margin-right: 10px;'>";
    html += "<input type='button' value='Cancel' onclick='window.history.back();'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/saveConfigBlynk", []() {
    String newBlynkTemplateID = server.arg("blynkTemplateID");      // รับค่าใหม่จากฟอร์ม
    String newBlynkTemplateName = server.arg("blynkTemplateName");  // รับค่าใหม่จากฟอร์ม
    String newBlynkAuthToken = server.arg("blynkAuthToken");        // รับค่าใหม่จากฟอร์ม
    // บันทึกค่าใหม่ลงใน Preferences
    preferences.begin("BlynkConfig", false);
    preferences.putString("templateID", newBlynkTemplateID);
    preferences.putString("templateName", newBlynkTemplateName);
    preferences.putString("authToken", newBlynkAuthToken);
    preferences.end();
    String html = "<html><body>";
    html += "<h1>BLYNK_TEMPLATE_ID saved!</h1>";
    html += "<p>Value: " + newBlynkTemplateID + "</p>";
    html += "<h1>BLYNK_TEMPLATE_NAME saved!</h1>";
    html += "<p>Value: " + newBlynkTemplateName + "</p>";
    html += "<h1>BLYNK_AUTH_TOKEN saved!</h1>";
    html += "<p>Value: " + newBlynkAuthToken + "</p>";
    html += "<p><a href='/'>Back to Menu</a></p>";  // ลิงก์กลับไปยังหน้าเมนู
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(500);
    Serial.println("Restarting ESP32...");
    ESP.restart();  // รีสตาร์ทหลังจากบันทึกค่า
  });

  server.on("/saveConfigParameter", []() {
    int newWetValue = server.arg("wetValue").toInt();                          // รับค่าใหม่จากฟอร์ม
    int newDryValue = server.arg("dryValue").toInt();                          // รับค่าใหม่จากฟอร์ม
    unsigned long newPumpRunTime = server.arg("pumpRunTime").toInt();          // รับค่าใหม่จากฟอร์ม
    unsigned long newCoolingDownTime = server.arg("coolingDownTime").toInt();  // รับค่าใหม่จากฟอร์ม
    // บันทึกค่าใหม่ลงใน Preferences
    preferences.begin("ConfigParameter", false);
    preferences.putInt("selectedWet", newWetValue);
    preferences.putInt("selectedDry", newDryValue);
    preferences.putLong("pumpRunTime", newPumpRunTime);
    preferences.putLong("coolingDownTime", newCoolingDownTime);
    preferences.end();
    String html = "<html><body>";
    html += "<h1>Max Wet Value saved!</h1>";
    html += "<p>Value: " + String(newWetValue) + "</p>";
    html += "<h1>Max Dry Value saved!</h1>";
    html += "<p>Value: " + String(newDryValue) + "</p>";
    html += "<h1>Max Pump RunTime saved!</h1>";
    html += "<p>Value: " + String(newPumpRunTime) + "</p>";
    html += "<h1>Pump CoolingDown Time saved!</h1>";
    html += "<p>Value: " + String(newCoolingDownTime) + "</p>";
    html += "<p><a href='/'>Back to Menu</a></p>";  // ลิงก์กลับไปยังหน้าเมนู
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(500);
    Serial.println("Restarting ESP32...");
    ESP.restart();  // รีสตาร์ทหลังจากบันทึกค่า
  });

  server.on("/save", []() {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    if (newSSID.length() > 0 && newPassword.length() > 0) {
      server.send(200, "text/html", "<h1>Configuration saved, attempting to connect...</h1>");
      // ลองเชื่อมต่อ WiFi ด้วย SSID และรหัสผ่านที่กรอกเข้ามา
      WiFi.begin(newSSID.c_str(), newPassword.c_str());
      int attempt = 0;
      const int max_attempts = 20;
      bool connected = false;
      // พยายามเชื่อมต่อ WiFi
      while (WiFi.status() != WL_CONNECTED && attempt < max_attempts) {
        delay(500);
        Serial.print(".");
        attempt++;
      }
      // ตรวจสอบสถานะการเชื่อมต่อ
      if (WiFi.status() == WL_CONNECTED) {
        // บันทึก SSID และ Password ลงในหน่วยความจำถาวร
        preferences.begin("WiFiCreds", false);
        preferences.putString("ssid", newSSID);
        preferences.putString("password", newPassword);
        preferences.end();

        Serial.println("\nConnected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        server.send(200, "text/html", "<h1>Configuration saved and connected!</h1>");
      }
      delay(500);
      Serial.println("Restarting ESP32...");
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid input, please try again</h1>");
    }
  });

  server.on("/setVeggie", []() {
    int veggieType = server.arg("veggie").toInt();
    Serial.print("Selected Veggie Type: ");
    Serial.println(veggieType);  // แสดงชนิดผักที่เลือกใน Serial Monitor
    // เก็บ veggieType ลงใน Preferences
    preferences.begin("VeggieType", false);
    preferences.putInt("selectedVeggie", veggieType);
    preferences.end();
    String html = "<html><body>";
    html += "<h1>Vegetable type selected!</h1>";
    html += "<p>Type: " + String(veggieType) + "</p>";
    html += "<p><a href='/'>Back to Menu</a></p>";  // ลิงก์กลับไปยังหน้าเมนู
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(500);
    Serial.println("Restarting ESP32...");
    ESP.restart();
  });
}

void loopServerSD() {
  server.handleClient();
}
