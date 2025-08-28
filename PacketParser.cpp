#include "PacketParser.h"

ParsedData parsePacket(const String& packet) {
  ParsedData d;
  String parts[11];
  int idx = 0, start = 0;
  for (int i = 0; i < packet.length(); i++) {
    if (packet[i] == ',' || i == packet.length() - 1) {
      int end = (i == packet.length() - 1) ? i + 1 : i;
      parts[idx++] = packet.substring(start, end);
      start = i + 1;
      if (idx >= 11) break;
    }
  }
  d.header = parts[0] + "," + parts[1];
  d.timestamp = parts[2];
  d.speed = parts[3].toInt();
  d.temperature = parts[4].toFloat();
  d.driverName = parts[5];
  d.licenseID = parts[6];
  d.continuousDrive = parts[7].toInt();
  d.dailyDrive = parts[8].toInt();
  d.engineOn = parts[9].toInt() == 0;
  d.checksum = parts[10].toInt();
  return d;
}