#include "smart_irrigation.h"

/*
+---------+------------+-------------+--------------+
| ผัก     | ปลูก (วัน) | รดน้ำ (%)   | หยุดรดน้ำ (%) |
+---------+------------+-------------+--------------+
| ผักชี   | 30-45      | 40          | 60           |
| กะเพรา  | 45-60      | 35          | 55           |
| โหระพา  | 45-60      | 40          | 60           |
| กาดขาว  | 50-60      | 50          | 70           |
| บุ้ง     | 25-30      | 40          | 60           |
| คะน้า   | 40-50      | 45          | 65           |
| หอม     | 30-40      | 35          | 55           |
| กาดหอม  | 40-50      | 50          | 70           |
| แก้ว    | 55-65      | 45          | 65           |
| แตงกวา  | 50-60      | 50          | 70           |
| พริก    | 60-90      | 35          | 55           |
+---------+------------+-------------+--------------+
*/

// ฟังก์ชันที่ใช้คืนค่าความชื้นที่ควรรดน้ำและควรหยุดรดน้ำ
void getMoistureRange(VeggieType veggie, int &threshold, int &stop) 
{
  switch(veggie) {
    case CILANTRO:
      threshold = 40; stop = 60;
      break;
    case BASIL:
      threshold = 35; stop = 55;
      break;
    case SWEET_BASIL:
      threshold = 40; stop = 60;
      break;
    case CHINESE_CABBAGE:
      threshold = 50; stop = 70;
      break;
    case MORNING_GLORY:
      threshold = 40; stop = 60;
      break;
    case KALE:
      threshold = 45; stop = 65;
      break;
    case SPRING_ONION:
      threshold = 35; stop = 55;
      break;
    case LETTUCE:
      threshold = 50; stop = 70;
      break;
    case ICEBERG_LETTUCE:
      threshold = 45; stop = 65;
      break;
    case CUCUMBER:
      threshold = 50; stop = 70;
      break;
    case CHILI:
      threshold = 35; stop = 55;
      break;
    default:
      threshold = 40; stop = 60; // Default case
  }
}
// นิยามฟังก์ชัน veggieToString
const char* veggieToString(VeggieType veggie) {
  switch (veggie) {
    case CILANTRO:         return "Cilantro";
    case BASIL:            return "Basil";
    case SWEET_BASIL:      return "Sweet Basil";
    case CHINESE_CABBAGE:  return "Chinese Cabbage";
    case MORNING_GLORY:    return "Morning Glory";
    case KALE:             return "Kale";
    case SPRING_ONION:     return "Spring Onion";
    case LETTUCE:          return "Lettuce";
    case ICEBERG_LETTUCE:  return "Iceberg Lettuce";
    case CUCUMBER:         return "Cucumber";
    case CHILI:            return "Chili";
    default:               return "Unknown";
  }
}
bool pumpStatus = false;

// ฟังก์ชันที่ใช้ตรวจสอบและควบคุมปั๊มน้ำ
bool checkPumpControl(VeggieType veggie, int soilMoisture) {
  int threshold, stop;
  getMoistureRange(veggie, threshold, stop);

  if (soilMoisture < threshold) {
    pumpStatus = true;  // เปิดปั๊มน้ำ
  } else if (soilMoisture > stop) {
    pumpStatus = false; // ปิดปั๊มน้ำ
  }
    
  return pumpStatus; // ไม่ทำอะไร ส่งค่าออกอย่างเดียว
}