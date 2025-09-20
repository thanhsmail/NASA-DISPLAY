#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "PacketParser.h"
#include "screen1.h"
#include "screen2.h"
#include "screen3.h" // ảnh nền cho màn hình cài đặt

#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

struct ScreenLayout {
  int timeX, timeY;
  int speedX, speedY;
  int tempX, tempY;
  int engineX, engineY;
  int contX, contY;
  int dailyX, dailyY;
  int driverX, driverY;
  int licenseX, licenseY;

  const GFXfont* fontTime;
  const GFXfont* fontSpeed;
  const GFXfont* fontTemp;
  const GFXfont* fontEngine;
  const GFXfont* fontCont;
  const GFXfont* fontDaily;
  const GFXfont* fontDriver;
  const GFXfont* fontLicense;

  bool showTime;
  bool showSpeed;
  bool showTemp;
  bool showEngine;
  bool showCont;
  bool showDaily;
  bool showDriver;
  bool showLicense;
};

enum ScreenType {
  SCREEN_INFO_1 = 0,
  SCREEN_INFO_2 = 1,
  SCREEN_SETTINGS = 2
};

class DisplayManager {
public:
  void init();
  void display(const ParsedData& data);
  void showLogo();

  void nextScreen();
  void setScreen(int index);
  int getScreenIndex();

  Adafruit_ILI9341* getTFT();

  void redrawProgressArea(int cx, int cy, int radius);

  // Phần cài đặt nhiệt độ
  void displaySettings(int tempMin, int tempMax, int selectedField);
  void showSettingsScreenOnce();

  // Đặt / lấy mục settings đang được chọn (nguồn sự thật)
  void setSettingsSelectedField(int f);
  int getSettingsSelectedField();

  // Biến min/max (load từ EEPROM)
  int tempMin = 20;
  int tempMax = 40;

  void clearLastText();
  void clearTextField(char* lastText, int x, int y, const GFXfont* font,
                      const uint16_t* bgImage, int bgImageWidth);

  int totalScreens = 3;
  int currentScreenIndex = 0;

private:
  Adafruit_ILI9341 tft = Adafruit_ILI9341(7, 10, 9);

  // Lưu text cũ để so sánh tránh vẽ lại
  char lastTime[32] = "";
  char lastSpeed[32] = "";
  char lastTemp[32] = "";
  char lastDriver[64] = "";
  char lastLicense[32] = "";
  char lastDrive1[32] = "";
  char lastDrive2[32] = "";
  char lastEngine[16] = "";

  // Lưu text cũ cho settings (để biết có cần vẽ lại)
  char lastMinText[32] = "";
  char lastMaxText[32] = "";

  bool settingsBgDrawn = false;

  // Trạng thái mục settings đang được chọn (lưu ở đây)
  int settingsSelectedField = -1;
  int lastSettingsField = -99; // để detect thay đổi chọn

  // Helpers
  uint16_t swapRedBlue(uint16_t color);
  void drawImageRegionRGB565_FAST(int16_t x, int16_t y, const uint16_t* data, int imgWidth,
                                  int16_t srcX, int16_t srcY, int regionW, int regionH);

  void displayUpdatedTextFontSmart(const char* newText, char* lastText, int x, int y,
                                   const GFXfont* font, uint16_t color,
                                   const uint16_t* bgImage, int bgImageWidth);

  // Ảnh nền cho từng màn hình
  const uint16_t* bgScreens[3] = { screen1, screen2, screen3 };
};

#endif
