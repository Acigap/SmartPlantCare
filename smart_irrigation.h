#include "Arduino.h"

// Enum ชนิดของผัก
enum VeggieType {
  CILANTRO,    // ผักชี
  BASIL,       // กะเพรา
  SWEET_BASIL, // โหระพา
  CHINESE_CABBAGE,  // ผักกาดขาว
  MORNING_GLORY,    // ผักบุ้ง
  KALE,             // คะน้า
  SPRING_ONION,     // ต้นหอม
  LETTUCE,          // ผักกาดหอม
  ICEBERG_LETTUCE,  // ผักกาดแก้ว
  CUCUMBER,          // แตงกวา
  CHILI             // พริก
};

void getMoistureRange(VeggieType veggie, int &threshold, int &stop);
const char* veggieToString(VeggieType veggie);
bool checkPumpControl(VeggieType veggie, int soilMoisture);