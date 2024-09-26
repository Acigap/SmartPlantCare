/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL6Iq1HOFra"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "vU9_myPzoMrSpRNUiiWZhlFZxo7qD5DG"

const char* host = "blynk.cloud";
int port = 80;
static int switchPum = 0;

#include <WiFi.h>
#include <WiFiClient.h>
#include "BlynkData.h"
#include <BlynkSimpleEsp32.h>

// WiFiClient client;

// bool httpRequest(const String& method,
//                  const String& url,
//                  const String& request,
//                  String&       response)
// {
//   Serial.print(F("Connecting to "));
//   Serial.print(host);
//   Serial.print(":");
//   Serial.print(port);
//   Serial.print("... ");
//   if (client.connect(host, port)) {
//     Serial.println("OK");
//   } else {
//     Serial.println("failed");
//     return false;
//   }

//   Serial.print(method); Serial.print(" "); Serial.println(url);

//   client.print(method); client.print(" ");
//   client.print(url); client.println(F(" HTTP/1.1"));
//   client.print(F("Host: ")); client.println(host);
//   client.println(F("Connection: close"));
//   if (request.length()) {
//     client.println(F("Content-Type: application/json"));
//     client.print(F("Content-Length: ")); client.println(request.length());
//     client.println();
//     client.print(request);
//   } else {
//     client.println();
//   }

//   //Serial.println("Waiting response");
//   int timeout = millis() + 5000;
//   while (client.available() == 0) {
//     if (timeout - millis() < 0) {
//       Serial.println(">>> Client Timeout !");
//       client.stop();
//       return false;
//     }
//   }

//   //Serial.println("Reading response");
//   int contentLength = -1;
//   while (client.available()) {
//     String line = client.readStringUntil('\n');
//     line.trim();
//     line.toLowerCase();
//     if (line.startsWith("content-length:")) {
//       contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
//     } else if (line.length() == 0) {
//       break;
//     }
//   }

//   //Serial.println("Reading response body");
//   response = "";
//   response.reserve(contentLength + 1);
//   while (response.length() < contentLength) {
//     if (client.available()) {
//       char c = client.read();
//       response += c;
//     } else if (!client.connected()) {
//       break;
//     }
//   }
//   client.stop();
//   Serial.print(response);
//   return true;
// }


// void isHardwareConnected() {
//   Serial.print("isHardwareConnected");
//   String response;
//   if (httpRequest("GET", String("/external/api/isHardwareConnected?token=") + BLYNK_AUTH_TOKEN, "", response)) {
//     if (response.length() != 0) {
//       Serial.print("WARNING: ");
//       Serial.println(response);
//     }
//   }
// }

// void virtualWrite(long value) {
//    // Send value to the cloud
//   // similar to Blynk.virtualWrite()

//   Serial.print("Sending value: ");
//   Serial.println(value);
//   String response;
//   if (httpRequest("GET", String("/external/api/update?token=") + BLYNK_AUTH_TOKEN + String("&pin=V2&value=") + value, "", response)) {
//     if (response.length() != 0) {
//       Serial.print("WARNING: ");
//       Serial.println(response);
//     }
//   }
// }

// String syncVirtual() {
//     // Read the value back
//   // similar to Blynk.syncVirtual()

//   String response;
//   Serial.println("Reading value");

//   if (httpRequest("GET", String("/external/api/get?token=") + BLYNK_AUTH_TOKEN + String("&pin=V2"), "", response)) {
//     Serial.print("Value from server: ");
//     Serial.println(response);
//   }
//   return response;
// }

// void setProperty(const String& label,
//                  const String& value) {
//                     // Set Property
//   Serial.println("Setting property");
//   String response;
//   if (httpRequest("GET", String("/external/api/update/property?token=") + BLYNK_AUTH_TOKEN + String("&pin=V3&") + label + "=" + value, "", response)) {
//     if (response.length() != 0) {
//       Serial.print("WARNING: ");
//       Serial.println(response);
//     }
//   }
// }

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

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(BLYNK_AUTH_TOKEN);  // in place of Blynk.begin(auth, ssid, pass);
    Blynk.connect(3333);  
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
