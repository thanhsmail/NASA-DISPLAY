#include <Arduino.h>
#include <EEPROM.h>
#include "MinMaxEEPROM.h"
#include "DisplayManager.h"
#include "PacketParser.h"
#include "DualButtonProgress.h"
#include "voice.h"
#define RXD2 20
#define TXD2 21
#define BTN_OK   5
#define BTN_DOWN  3
#define BTN_UP    8

//HardwareSerial SerialRS232(1);
String buffer = ""; 

DisplayManager displayMgr;
DualButtonProgress dbp(BTN_OK, BTN_DOWN, BTN_UP, &displayMgr);

int screenChangeCount = 0;

void changeScreenCallback() {
  screenChangeCount++;
  if (screenChangeCount == 3) {
    displayMgr.setScreen(SCREEN_SETTINGS);
    screenChangeCount = 0;
  } else {
    int idx = displayMgr.getScreenIndex();
    if (idx == SCREEN_SETTINGS) {
      displayMgr.setScreen(0);
    } else {
      displayMgr.nextScreen();
    }
  }
}

void setup() {
    Serial.begin(115200);
  #if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
  #endif
  //SerialRS232.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial0.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("HELLO NASA!");
  setupVoiceAlert();

  // Đọc MIN/MAX từ EEPROM khi khởi động
  loadMinMaxFromEEPROM(displayMgr.tempMin, displayMgr.tempMax);

  displayMgr.init();
  displayMgr.showLogo();

  dbp.begin();
  dbp.setHoldTime(2000);
  dbp.onComplete(changeScreenCallback);
  String fakePacket3 = "!NASA,1,2025-07-27 21:00:00,99,-10.69,HO VA TEN,GPLX123456,30,630,1,123";
  ParsedData fakeData3 = parsePacket(fakePacket3);
  Serial.println(fakePacket3);
  displayMgr.display(fakeData3);
}

void loop() {
  handleVirtualSerialInput();
  dbp.update();

  if (Serial.available() && displayMgr.getScreenIndex() != SCREEN_SETTINGS) {
    String packet = Serial.readStringUntil('\r');
    ParsedData info = parsePacket(packet);
    displayMgr.display(info);
  }

  while (Serial0.available()&& displayMgr.getScreenIndex() != SCREEN_SETTINGS) {
    char c = Serial0.read();
    if (c == '\n' || c == '\r') {
      if (buffer.length() > 0) {
        buffer.trim();
        Serial.println("Received: " + buffer);
        ParsedData info = parsePacket(buffer);
        displayMgr.display(info);
        buffer = "";
      }
    } else {
      buffer += c;
    }
  }
  loopVoiceAlert();
  delay(50);
}

void handleVirtualSerialInput() {
  if (Serial.available() && displayMgr.getScreenIndex() != SCREEN_SETTINGS) {
    String packet = Serial.readStringUntil('\r');
    if (packet.startsWith("!NASA")) {
      Serial.println("[DEBUG] Received virtual packet: " + packet);
      ParsedData info = parsePacket(packet);
      // Cảnh báo khi nhiệt độ vượt ngoài set
      if (info.temperature < displayMgr.tempMin) {
        //playVoiceAlertLowTemp();
        Serial.println("nhiet độ thấp");
      } else if (info.temperature > displayMgr.tempMax) {
         Serial.println("nhiet độ cao");
        //playVoiceAlertHighTemp();
      }
      displayMgr.display(info);
    }
  }
}