#include "DisplayManager.h"
#include <Arduino.h>
#include "voice.h"
static char lastMinText[16] = "";
static char lastMaxText[16] = "";
static bool blinkState = false;
static unsigned long lastBlinkTime = 0;
static const unsigned long BLINK_INTERVAL = 350;  // ms
static int lastBlinkField = -1;                   // Lưu trạng thái trường đã chọn để xoá dấu >>
// ===== Layout cho từng màn hình =====
const ScreenLayout layouts[3] = {
  // Layout Screen 1: Hiển thị một số thông số, ẩn một số khác
  { 30, 20, 100, 158, 110, 100, 203, 160, 20, 80, 20, 140, 60, 195, 60, 225,
    &FreeSansBoldOblique9pt7b,  // fontTime
    &FreeSansBoldOblique9pt7b,  // fontSpeed
    &FreeSansBold18pt7b,        // fontTemp
    &FreeSansBoldOblique9pt7b,  // fontEngine
    &FreeSansBoldOblique9pt7b,  // fontCont
    &FreeSansBoldOblique9pt7b,  // fontDaily
    &FreeSansBoldOblique9pt7b,  // fontDriver
    &FreeSansBoldOblique9pt7b,  // fontLicens
    // Các cờ hiển thị cho màn hình 1
    true, true, true, true, true, true, true, true },
  //times,speed,temp,engine,4h,10,lai xe,ID
  // Layout Screen 2: Hiển thị các thông số khác
  { 30, 20,              // timeX, timeY
    30, 90,             // speedX, speedY
    90, 50,              // tempX, tempY
    15, 150,            // engineX, engineY
    160, 80,              // contX, contY
    160, 150,             // dailyX, dailyY
    40, 195,             // driverX, driverY
    40, 225,             // licenseX, licenseY
    &FreeMonoBold9pt7b,  // fontTime
    &FreeSans9pt7b,      // fontSpeed
    &FreeSans9pt7b,      // fontTemp
    &FreeMonoBold9pt7b,  // fontEngine
    &FreeSansBoldOblique9pt7b,  // fontCont
    &FreeSansBoldOblique9pt7b,  // fontDaily
    &FreeMonoBold9pt7b,  // fontDriver
    &FreeMonoBold9pt7b,  // fontLicense
    // Các cờ hiển thị cho màn hình 2
    true, true, false, true, true, true, true, true },
  // Layout Screen 3: Hiển thị các thông số khác
    // { //Layout Screen 3
    //   // Chọn vị trí bạn muốn cho các thông số hoặc copy layout của màn 1/2 rồi chỉnh lại
    //   30, 20,                     // timeX, timeY
    //   100, 158,                   // speedX, speedY
    //   110, 100,                   // tempX, tempY
    //   203, 160,                   // engineX, engineY
    //   20, 80,                     // contX, contY
    //   20, 140,                    // dailyX, dailyY
    //   60, 195,                    // driverX, driverY
    //   60, 225,                    // licenseX, licenseY
    //   &FreeSansBoldOblique9pt7b,  // fontTime
    //   &FreeSansBoldOblique9pt7b,  // fontSpeed
    //   &FreeSansBold18pt7b,        // fontTemp
    //   &FreeSansBoldOblique9pt7b,  // fontEngine
    //   &FreeSansBoldOblique9pt7b,  // fontCont
    //   &FreeSansBoldOblique9pt7b,  // fontDaily
    //   &FreeSansBoldOblique9pt7b,  // fontDriver
    //   &FreeSansBoldOblique9pt7b,  // fontLicense
    //   // Các cờ hiển thị cho màn hình 3 (tùy ý: true/false)
    //   true, false, true, false, false, false, false, false }
};

void DisplayManager::init() {
  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);
}

void DisplayManager::showLogo() {
  drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
  clearLastText();  // Reset text sau khi vẽ logo
}

uint16_t DisplayManager::swapRedBlue(uint16_t color) {
  uint16_t r = (color & 0xF800) >> 11;
  uint16_t g = (color & 0x07E0);
  uint16_t b = (color & 0x001F);
  return (b << 11) | g | r;
}

void DisplayManager::drawImageRegionRGB565_FAST(int16_t x, int16_t y, const uint16_t* data, int imgWidth,
                                                int16_t srcX, int16_t srcY, int regionW, int regionH) {
  tft.startWrite();
  tft.setAddrWindow(x, y, regionW, regionH);
  for (int16_t j = 0; j < regionH; j++) {
    for (int16_t i = 0; i < regionW; i++) {
      uint16_t color = swapRedBlue(pgm_read_word(data + (srcY + j) * imgWidth + (srcX + i)));
      tft.writeColor(color, 1);
    }
  }
  tft.endWrite();
}

void DisplayManager::displayUpdatedTextFontSmart(const char* newText, char* lastText, int x, int y,
                                                 const GFXfont* font, uint16_t color,
                                                 const uint16_t* bgImage, int bgImageWidth) {
  if (strcmp(newText, lastText) != 0) {
    tft.setFont(font);
    tft.setTextSize(1);

    int16_t x1_old, y1_old;
    uint16_t w_old, h_old;
    tft.getTextBounds(lastText, x, y, &x1_old, &y1_old, &w_old, &h_old);

    const uint8_t padding = 4;
    x1_old = max(0, x1_old - padding);
    y1_old = max(0, y1_old - padding);
    w_old += padding * 2;
    h_old += padding * 2;

    drawImageRegionRGB565_FAST(x1_old, y1_old, bgImage, bgImageWidth, x1_old, y1_old, w_old, h_old);

    tft.setCursor(x, y);
    tft.setTextColor(swapRedBlue(color));
    tft.print(newText);

    strcpy(lastText, newText);
    tft.setFont();
  }
}

void DisplayManager::display(const ParsedData& info) {
  const uint16_t* bg = bgScreens[currentScreenIndex];
  const ScreenLayout& L = layouts[currentScreenIndex];

  // Nếu là màn settings, chỉ vẽ nền nếu chưa vẽ
  if (currentScreenIndex == SCREEN_SETTINGS) {
    showSettingsScreenOnce();  // chỉ vẽ nền khi chưa vẽ
    // KHÔNG vẽ MIN/MAX ở đây nữa
  } else {
    settingsBgDrawn = false;
  }

  // 1. HIỂN THỊ CÁC THÔNG SỐ layouts trước
  if (L.showTime) displayUpdatedTextFontSmart(info.timestamp.c_str(), lastTime, L.timeX, L.timeY, L.fontTime, ILI9341_BLACK, bg, 320);
  else clearTextField(lastTime, L.timeX, L.timeY, L.fontTime, bg, 320);

  if (L.showSpeed) displayUpdatedTextFontSmart((String(info.speed) + " km/h").c_str(), lastSpeed, L.speedX, L.speedY, L.fontSpeed, ILI9341_RED, bg, 320);
  else clearTextField(lastSpeed, L.speedX, L.speedY, L.fontSpeed, bg, 320);

  if (L.showTemp) displayUpdatedTextFontSmart(String(info.temperature).c_str(), lastTemp, L.tempX, L.tempY, L.fontTemp, ILI9341_GREEN, bg, 320);
  else clearTextField(lastTemp, L.tempX, L.tempY, L.fontTemp, bg, 320);

  if (L.showEngine) {
    const char* engineText = info.engineOn ? "ON" : "OFF";
    uint16_t engineColor = info.engineOn ? ILI9341_GREEN : ILI9341_RED;
    displayUpdatedTextFontSmart(engineText, lastEngine, L.engineX, L.engineY, L.fontEngine, engineColor, bg, 320);
  } else clearTextField(lastEngine, L.engineX, L.engineY, L.fontEngine, bg, 320);

  if (L.showCont) {
    char contStr[10];
    snprintf(contStr, sizeof(contStr), "%02d:%02d", info.continuousDrive / 60, info.continuousDrive % 60);
    uint16_t contColor = (info.continuousDrive > 240) ? ILI9341_RED : ILI9341_BLUE;
    displayUpdatedTextFontSmart(contStr, lastDrive1, L.contX, L.contY, L.fontCont, contColor, bg, 320);
  } else clearTextField(lastDrive1, L.contX, L.contY, L.fontCont, bg, 320);

  if (L.showDaily) {
    char dailyStr[10];
    snprintf(dailyStr, sizeof(dailyStr), "%02d:%02d", info.dailyDrive / 60, info.dailyDrive % 60);
    uint16_t dailyColor = (info.dailyDrive > 240) ? ILI9341_RED : ILI9341_BLUE;
    displayUpdatedTextFontSmart(dailyStr, lastDrive2, L.dailyX, L.dailyY, L.fontDaily, dailyColor, bg, 320);
  } else clearTextField(lastDrive2, L.dailyX, L.dailyY, L.fontDaily, bg, 320);

  if (L.showDriver) displayUpdatedTextFontSmart(info.driverName.c_str(), lastDriver, L.driverX, L.driverY, L.fontDriver, ILI9341_BLUE, bg, 320);
  else clearTextField(lastDriver, L.driverX, L.driverY, L.fontDriver, bg, 320);

  if (L.showLicense) displayUpdatedTextFontSmart(info.licenseID.c_str(), lastLicense, L.licenseX, L.licenseY, L.fontLicense, ILI9341_BLUE, bg, 320);
  else clearTextField(lastLicense, L.licenseX, L.licenseY, L.fontLicense, bg, 320);

  // 2. SAU ĐÓ vẽ giao diện MIN/MAX
  if (currentScreenIndex == SCREEN_SETTINGS) {
    displaySettings(tempMin, tempMax, -1);  // vẽ MIN/MAX và hiệu ứng
  }

  // // Cảnh báo liên quan đến lái xe (phần này giữ nguyên)
  // if (info.speed > 3) {
  //   if (info.continuousDrive >= 240) playVoiceAlertDrive4h();
  //   else if (info.continuousDrive >= 225) playVoiceAlertSoon4h();
  //   if (info.dailyDrive >= 600) playVoiceAlertDrive10h();
  //   else if (info.dailyDrive >= 585) playVoiceAlertSoon10h();
  // }
}

// Khi chuyển màn hình, reset lại text cũ
void DisplayManager::setScreen(int index) {
  if (index >= 0 && index < totalScreens) {
    currentScreenIndex = index;
    settingsBgDrawn = false;
    drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
    clearLastText();
    strcpy(lastMinText, "");
    strcpy(lastMaxText, "");
  }
}

void DisplayManager::nextScreen() {
  currentScreenIndex = (currentScreenIndex + 1) % totalScreens;
  settingsBgDrawn = false;
  drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
  clearLastText();
  strcpy(lastMinText, "");
  strcpy(lastMaxText, "");
}
int DisplayManager::getScreenIndex() {
  return currentScreenIndex;
}

Adafruit_ILI9341* DisplayManager::getTFT() {
  return &tft;
}
// Hàm mới để vẽ lại vùng ảnh nền
void DisplayManager::redrawProgressArea(int cx, int cy, int radius) {
  const uint16_t* bg = bgScreens[currentScreenIndex];
  int imgWidth = 320;  // Chiều rộng của ảnh nền

  // Tính toán vùng cần vẽ lại (hình vuông bao quanh hình tròn)
  int16_t x = max(0, cx - radius);
  int16_t y = max(0, cy - radius);
  int regionW = min(imgWidth - x, radius * 2);
  int regionH = min(240 - y, radius * 2);  // Giả sử chiều cao màn hình là 240

  drawImageRegionRGB565_FAST(x, y, bg, imgWidth, x, y, regionW, regionH);
}
// Định nghĩa hàm để xóa các biến text cũ
void DisplayManager::clearLastText() {
  strcpy(lastTime, "");
  strcpy(lastSpeed, "");
  strcpy(lastTemp, "");
  strcpy(lastDriver, "");
  strcpy(lastLicense, "");
  strcpy(lastDrive1, "");
  strcpy(lastDrive2, "");
  strcpy(lastEngine, "");
}

void DisplayManager::clearTextField(char* lastText, int x, int y, const GFXfont* font, const uint16_t* bgImage, int bgImageWidth) {
  if (strcmp(lastText, "") != 0) {
    tft.setFont(font);
    tft.setTextSize(1);
    int16_t x1_old, y1_old;
    uint16_t w_old, h_old;
    tft.getTextBounds(lastText, x, y, &x1_old, &y1_old, &w_old, &h_old);

    const uint8_t padding = 4;
    x1_old = max(0, x1_old - padding);
    y1_old = max(0, y1_old - padding);
    w_old += padding * 2;
    h_old += padding * 2;

    drawImageRegionRGB565_FAST(x1_old, y1_old, bgImage, bgImageWidth, x1_old, y1_old, w_old, h_old);

    strcpy(lastText, "");
    tft.setFont();
  }
}


void DisplayManager::showSettingsScreenOnce() {
  if (!settingsBgDrawn) {
    drawImageRegionRGB565_FAST(0, 0, bgScreens[SCREEN_SETTINGS], 320, 0, 0, 320, 240);
    settingsBgDrawn = true;
  }
}

void DisplayManager::displaySettings(int tempMin, int tempMax, int selectedField) {
  showSettingsScreenOnce();

  tft.setFont(&FreeSansBoldOblique9pt7b);
  tft.setTextColor(ILI9341_BLUE);
  tft.setCursor(40, 60);
  tft.print("CAI DAT NHIET DO");

  // Min
  char minBuf[16];
  snprintf(minBuf, sizeof(minBuf), "MIN: %d C", tempMin);
  displayUpdatedTextFontSmart(
    minBuf,
    lastMinText,
    70, 100,  // Vị trí sau dấu >>
    &FreeSans9pt7b,
    selectedField == 0 ? ILI9341_RED : ILI9341_WHITE,
    bgScreens[SCREEN_SETTINGS],
    320);

  // Max
  char maxBuf[16];
  snprintf(maxBuf, sizeof(maxBuf), "MAX: %d C", tempMax);
  displayUpdatedTextFontSmart(
    maxBuf,
    lastMaxText,
    70, 150,  // Vị trí sau dấu >>
    &FreeSans9pt7b,
    selectedField == 1 ? ILI9341_RED : ILI9341_WHITE,
    bgScreens[SCREEN_SETTINGS],
    320);

  //Hiệu ứng nhấp nháy
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  // Xóa triệt để dấu >> ở trường cũ khi chuyển trường
  if (lastBlinkField != selectedField) {
    // Xóa vùng dấu >> ở cả hai trường
    drawImageRegionRGB565_FAST(40, 90, bgScreens[SCREEN_SETTINGS], 320, 50, 95, 30, 20);    // Min
    drawImageRegionRGB565_FAST(40, 140, bgScreens[SCREEN_SETTINGS], 320, 50, 145, 30, 20);  // Max
    lastBlinkField = selectedField;
  }

  // Vẽ dấu >> nhấp nháy chỉ ở trường đang chỉnh khi blinkState == true
  tft.setFont(&FreeSans9pt7b);
  if (blinkState) {
    if (selectedField == 0) {
      tft.setTextColor(swapRedBlue(ILI9341_YELLOW));
      tft.setCursor(50, 100);
      tft.print(">>");
    } else if (selectedField == 1) {
      tft.setTextColor(ILI9341_YELLOW);
      tft.setCursor(50, 150);
      tft.print(">>");
    }
  } else {
    // Xóa triệt để dấu >> khi blinkState == false (chính xác vùng)
    if (selectedField == 0) {
      drawImageRegionRGB565_FAST(40, 90, bgScreens[SCREEN_SETTINGS], 320, 50, 95, 30, 20);  // Min
    } else if (selectedField == 1) {
      drawImageRegionRGB565_FAST(40, 140, bgScreens[SCREEN_SETTINGS], 320, 50, 145, 30, 20);  // Max
    }
  }
  tft.setFont();
}