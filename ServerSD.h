#include "Arduino.h"
#include <Preferences.h>  // เพิ่มการเรียกใช้ Preferences library

extern const char *csvFilename;
extern const char *apIP;
extern Preferences preferences;

void setupServerSD(const char *host,void (*callback)(String, String, String));
void loopServerSD();
void startSDServer();
void startAPMode();
void startHostName(const char *host);
void startWifiServer();