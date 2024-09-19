#include "Arduino.h"

void createDir(const char *path);
void removeDir(const char *path);
void readFile(const char *path);
String readFileToString(const char *path);
void writeFile(const char *path, const char *message);
void appendFile(const char *path, const char *message);
void renameFile(const char *path1, const char *path2);
void deleteFile(const char *path);
void setupSDCard();

/* External expansion */
#define PIN_SD_CMD    13
#define PIN_SD_CLK    11
#define PIN_SD_D0     12