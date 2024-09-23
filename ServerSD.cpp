#include <ESPmDNS.h> // สำหรับ จัดการ host name
#include "ServerSD.h"
#include <WiFi.h>  
#include <WebServer.h> // สำหรับ Web Server
#include "FS.h"  // สำหรับจัดการไฟล์
#include "SD_MMC.h" // สำหรับจัดการ SD card

const char *csvFilename = "/log.csv";
const char *hostLocal;
const char *apName = "MyPlant";
const char *apPWD = "12345678";

WebServer server(80);
Preferences preferences;

// ตัวแปร global เพื่อเก็บ callback ที่รับเข้ามา
void (*storedCallback)(String, String, String) = nullptr;

void setupServerSD(const char *host, void (*callback)(String,String,String)) {
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
      startHostName(host);
      // รอให้ผู้ใช้ตั้งค่า WiFi ใหม่
      while (WiFi.status() != WL_CONNECTED) {
          server.handleClient();  // รอให้ผู้ใช้กรอก SSID และ Password ใหม่
          delay(100);
      }
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
  // เริ่ม Web Server
  server.on("/", []() {
      String html = "<html><body>";
      html += "<h1>WiFi Configuration</h1>";
      html += "<form action='/save' method='POST'>";
      html += "SSID: <input type='text' name='ssid'><br>";
      html += "Password: <input type='text' name='password'><br>";
      html += "<input type='submit' value='Save'>";
      html += "</form></body></html>";
      server.send(200, "text/html", html);
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
      // รีสตาร์ท ESP32
      Serial.println("Restarting ESP32...");
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid input, please try again</h1>");
    }
  });
}

void loopServerSD() {
    server.handleClient();
}