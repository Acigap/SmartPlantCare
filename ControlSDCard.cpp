#include "FS.h"  // สำหรับจัดการไฟล์
#include "SD_MMC.h" // สำหรับจัดการ SD card
#include "ControlSDCard.h"

void createDir(const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (SD_MMC.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (SD_MMC.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = SD_MMC.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
}

String readFileToString(const char *path)
{
  File file = SD_MMC.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  
  String s = "";
  while (file.available()){
    s +=  (char)file.read();
  }
  return s;
}

void writeFile(const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
}

void appendFile(const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = SD_MMC.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for Appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
}

void renameFile(const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (SD_MMC.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (SD_MMC.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void setupSDCard() {
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  if (!SD_MMC.begin("/sdcard", true, true)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
}