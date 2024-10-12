#include "Arduino.h"

String readFileToString(const char *path);
void writeFile(const char *path, const char *message);
void appendFile(const char *path, const char *message);
void renameFile(const char *path1, const char *path2);
void setupSDCard();

/* External expansion */
#define PIN_SD_CMD    13
#define PIN_SD_CLK    11
#define PIN_SD_D0     12