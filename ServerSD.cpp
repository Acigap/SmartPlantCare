#include <ESPmDNS.h> // สำหรับ จัดการ host name
#include "ServerSD.h"
#include <WiFi.h>  
#include <WebServer.h> // สำหรับ Web Server
#include "FS.h"  // สำหรับจัดการไฟล์
#include "SD_MMC.h" // สำหรับจัดการ SD card

const char *csvFilename = "/log.csv";

WebServer server(80);

void setupServerSD(const char *host) {
  Serial.setDebugOutput(false);
  Serial.print("\n");

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {  //wait 10 seconds
    delay(500);
  }

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }

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
  server.begin();
}

void loopServerSD() {
    server.handleClient();
}