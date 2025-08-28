#pragma once
#include <EEPROM.h>

// Địa chỉ lưu MIN/MAX trong EEPROM (4 bytes/int)
#define EEPROM_ADDR_MIN 0
#define EEPROM_ADDR_MAX 4

inline void saveMinMaxToEEPROM(int minVal, int maxVal) {
    EEPROM.put(EEPROM_ADDR_MIN, minVal);
    EEPROM.put(EEPROM_ADDR_MAX, maxVal);
    #if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
    #endif
}

inline void loadMinMaxFromEEPROM(int &minVal, int &maxVal) {
    EEPROM.get(EEPROM_ADDR_MIN, minVal);
    EEPROM.get(EEPROM_ADDR_MAX, maxVal);
    // Nếu giá trị ngoài vùng, đặt mặc định
    if (minVal < -100 || minVal > 100) minVal = 20;
    if (maxVal < -100 || maxVal > 100) maxVal = 40;
}