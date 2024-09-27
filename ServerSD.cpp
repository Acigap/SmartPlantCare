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
    html += "<h1>Menu</h1>";
    html += "<p>Mode: " + String(WiFi.softAPIP().toString() != "0.0.0.0" ? "AP Mode" : "WiFi Connected") + "</p>";
    html += "<p>SSID: " + String(WiFi.SSID()) == "" ? String(apName) : String(WiFi.SSID()) + "</p>";
    html += "<h2>Select an Option</h2>";
    html += "<ul>";
    html += "<li><a href='/setup'>Setup WiFi</a></li>";
    html += "<li><a href='/download'>Download CSV Log</a></li>";
    html += "<li><a href='/veggieSelection'>Select Veggie Type</a></li>";
    html += "<li><a href='/setWetValue'>Set Wet Value</a></li>";  // เพิ่มหน้าใหม่สำหรับการตั้งค่า wetValue
    html += "</ul>";
    html += "</div></body></html>";
    server.send(200, "text/html", html);
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
    html += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>Select Veggie Type</h1>";
    html += "<form action='/setVeggie' method='POST'>";
    // Select dropdown with default selection based on veggieType
    html += "<select name='veggie'>";
    html += "<option value='0'" + String(veggieType == 0 ? " selected" : "") + ">ผักชี</option>";
    html += "<option value='1'" + String(veggieType == 1 ? " selected" : "") + ">กะเพรา</option>";
    html += "<option value='2'" + String(veggieType == 2 ? " selected" : "") + ">โหระพา</option>";
    html += "<option value='3'" + String(veggieType == 3 ? " selected" : "") + ">ผักกาดขาว</option>";
    html += "<option value='4'" + String(veggieType == 4 ? " selected" : "") + ">ผักบุ้ง</option>";
    html += "<option value='5'" + String(veggieType == 5 ? " selected" : "") + ">คะน้า</option>";
    html += "<option value='6'" + String(veggieType == 6 ? " selected" : "") + ">ต้นหอม</option>";
    html += "<option value='7'" + String(veggieType == 7 ? " selected" : "") + ">ผักกาดหอม</option>";
    html += "<option value='8'" + String(veggieType == 8 ? " selected" : "") + ">ผักกาดแก้ว</option>";
    html += "<option value='9'" + String(veggieType == 9 ? " selected" : "") + ">แตงกวา</option>";
    html += "<option value='10'" + String(veggieType == 10 ? " selected" : "") + ">พริก</option>";

    html += "</select><br>";
    html += "<input type='submit' value='Select'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/setup", []() {
    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "input[type='text'], input[type='password'] { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; width: 100%; }";
    html += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form action='/save' method='POST'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Save'></form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/setWetValue", []() {
    // อ่านค่า wetValue จาก Preferences
    preferences.begin("WetValue", false);
    int wetValue = preferences.getInt("selectedWet", 0);  // ค่าเริ่มต้นคือ 0
    unsigned long pumpRunTime = preferences.getLong("pumpRunTime", 0);  // ค่าเริ่มต้นคือ 0
    unsigned long coolingDownTime = preferences.getLong("coolingDownTime", 0);  // ค่าเริ่มต้นคือ 0
    preferences.end();

    Serial.println("WetValuePrefs: ");
    Serial.print(wetValue);

    String html = "<html lang='th'><head><meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; color: #333; }";
    html += "h1 { color: #5a5a5a; }";
    html += "form { background: #fff; padding: 20px; border-radius: 5px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
    html += "input[type='number'] { padding: 10px; margin-top: 10px; border-radius: 5px; border: 1px solid #ccc; width: 100%; }";
    html += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>Set Wet Value</h1>";
    html += "<form action='/saveWetValue' method='POST'>";
    html += "Wet Value: <input type='number' name='wetValue' value='" + String(wetValue) + "'><br>";  // ตั้งค่า default
    html += "Max Pump RunTime: <input type='number' name='pumpRunTime' value='" + String(pumpRunTime) + "'><br>";  // ตั้งค่า default
    html += "Pump CoolingDown Time: <input type='number' name='coolingDownTime' value='" + String(coolingDownTime) + "'><br>";  // ตั้งค่า default
    html += "<input type='submit' value='Save'></form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/saveWetValue", []() {
    int newWetValue = server.arg("wetValue").toInt();  // รับค่าใหม่จากฟอร์ม
    unsigned long newPumpRunTime = server.arg("pumpRunTime").toInt();  // รับค่าใหม่จากฟอร์ม
    unsigned long newCoolingDownTime = server.arg("coolingDownTime").toInt();  // รับค่าใหม่จากฟอร์ม
    Serial.print("New Wet Value: ");
    Serial.println(newWetValue);

    // บันทึกค่าใหม่ลงใน Preferences
    preferences.begin("WetValue", false);
    preferences.putInt("selectedWet", newWetValue);
    preferences.putLong("pumpRunTime", newPumpRunTime);
    preferences.putLong("coolingDownTime", newCoolingDownTime);
    preferences.end();

    String html = "<html><body>";
    html += "<h1>Wet Value saved!</h1>";
    html += "<p>Value: " + String(newWetValue) + "</p>";
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
      Serial.print("New SSID: ");
      Serial.println(newSSID);
      Serial.print("New Password: ");
      Serial.println(newPassword);

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
