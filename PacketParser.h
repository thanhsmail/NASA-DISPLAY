#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

#include <Arduino.h>

struct ParsedData {
  String header, timestamp, driverName, licenseID;
  int speed, continuousDrive, dailyDrive, checksum;
  float temperature;
  bool engineOn;
};

ParsedData parsePacket(const String& packet);

#endif