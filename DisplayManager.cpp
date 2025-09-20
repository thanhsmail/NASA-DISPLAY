#include "DisplayManager.h"
#include <Arduino.h>
#include "voice.h"

// thời gian để refresh (nếu cần)
static const unsigned long BLINK_INTERVAL = 350;  // không dùng blink ở đây

// ===== Layout =====
const ScreenLayout layouts[3] = {
  // Screen 1
  { 30, 20, 100, 158, 110, 100, 203, 160, 20, 80, 20, 140, 60, 195, 60, 225,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b, &FreeSansBold18pt7b,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b,
    true, true, true, true, true, true, true, true },

  // Screen 2
  { 30, 20, 30, 90, 90, 50, 15, 150, 160, 80, 160, 150, 40, 195, 40, 225,
    &FreeMonoBold9pt7b, &FreeSans9pt7b, &FreeSans9pt7b,
    &FreeMonoBold9pt7b, &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b,
    &FreeMonoBold9pt7b, &FreeMonoBold9pt7b,
    true, true, false, true, true, true, true, true },

  // Screen 3 (settings background)
  { 30, 20, 100, 158, 110, 100, 203, 160, 20, 80, 20, 140, 60, 195, 60, 225,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b, &FreeSansBold18pt7b,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b,
    &FreeSansBoldOblique9pt7b, &FreeSansBoldOblique9pt7b,
    true, false, false, false, false, false, false, false }
};

// ================ Init ================
void DisplayManager::init() {
  tft.begin();
  tft.setRotation(2);
  // Vẽ nền màn hình hiện tại ngay khi init
  drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
  clearLastText();
}

void DisplayManager::showLogo() {
  drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
  clearLastText();
}

// ================ Helpers ================
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
  // Nếu text khác thì vẽ lại
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
    return;
  }
  // Nếu text không đổi nhưng chúng ta cần đổi màu (ví dụ selection thay đổi),
  // caller nên clearTextField(...) trước rồi gọi hàm này. (chúng ta không ép re-draw ở đây)
}

// ================ Display ================
void DisplayManager::display(const ParsedData& info) {
  const uint16_t* bg = bgScreens[currentScreenIndex];
  const ScreenLayout& L = layouts[currentScreenIndex];

  // Nếu là màn settings → chỉ vẽ nền + tiêu đề 1 lần
  if (currentScreenIndex == SCREEN_SETTINGS) {
    showSettingsScreenOnce();
  } else {
    settingsBgDrawn = false;
  }

  // Time
  if (L.showTime) displayUpdatedTextFontSmart(info.timestamp.c_str(), lastTime, L.timeX, L.timeY, L.fontTime, ILI9341_BLACK, bg, 320);
  else clearTextField(lastTime, L.timeX, L.timeY, L.fontTime, bg, 320);

  // Speed
  if (L.showSpeed) displayUpdatedTextFontSmart((String(info.speed) + " km/h").c_str(), lastSpeed, L.speedX, L.speedY, L.fontSpeed, ILI9341_RED, bg, 320);
  else clearTextField(lastSpeed, L.speedX, L.speedY, L.fontSpeed, bg, 320);

  // Temp
  if (L.showTemp) displayUpdatedTextFontSmart(String(info.temperature).c_str(), lastTemp, L.tempX, L.tempY, L.fontTemp, ILI9341_GREEN, bg, 320);
  else clearTextField(lastTemp, L.tempX, L.tempY, L.fontTemp, bg, 320);

  // Engine
  if (L.showEngine) {
    const char* engineText = info.engineOn ? "ON" : "OFF";
    uint16_t engineColor = info.engineOn ? ILI9341_GREEN : ILI9341_RED;
    displayUpdatedTextFontSmart(engineText, lastEngine, L.engineX, L.engineY, L.fontEngine, engineColor, bg, 320);
  } else clearTextField(lastEngine, L.engineX, L.engineY, L.fontEngine, bg, 320);

  // continuous
  if (L.showCont) {
    char contStr[10];
    snprintf(contStr, sizeof(contStr), "%02d:%02d", info.continuousDrive / 60, info.continuousDrive % 60);
    uint16_t contColor = (info.continuousDrive > 240) ? ILI9341_RED : ILI9341_BLUE;
    displayUpdatedTextFontSmart(contStr, lastDrive1, L.contX, L.contY, L.fontCont, contColor, bg, 320);
  } else clearTextField(lastDrive1, L.contX, L.contY, L.fontCont, bg, 320);

  // daily
  if (L.showDaily) {
    char dailyStr[10];
    snprintf(dailyStr, sizeof(dailyStr), "%02d:%02d", info.dailyDrive / 60, info.dailyDrive % 60);
    uint16_t dailyColor = (info.dailyDrive > 240) ? ILI9341_RED : ILI9341_BLUE;
    displayUpdatedTextFontSmart(dailyStr, lastDrive2, L.dailyX, L.dailyY, L.fontDaily, dailyColor, bg, 320);
  } else clearTextField(lastDrive2, L.dailyX, L.dailyY, L.fontDaily, bg, 320);

  // driver & license
  if (L.showDriver) displayUpdatedTextFontSmart(info.driverName.c_str(), lastDriver, L.driverX, L.driverY, L.fontDriver, ILI9341_BLUE, bg, 320);
  else clearTextField(lastDriver, L.driverX, L.driverY, L.fontDriver, bg, 320);

  if (L.showLicense) displayUpdatedTextFontSmart(info.licenseID.c_str(), lastLicense, L.licenseX, L.licenseY, L.fontLicense, ILI9341_BLUE, bg, 320);
  else clearTextField(lastLicense, L.licenseX, L.licenseY, L.fontLicense, bg, 320);

  // Nếu đang settings, cho phép vẽ riêng (ở displayTask bạn nên gọi displaySettings() liên tục khi đang ở settings)
}

// ================ Screen control ================
void DisplayManager::setScreen(int index) {
  if (index >= 0 && index < totalScreens) {
    currentScreenIndex = index;
    settingsBgDrawn = false;
    drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
    clearLastText();
    lastMinText[0] = '\0';
    lastMaxText[0] = '\0';
    // reset selection when leaving settings
    if (currentScreenIndex != SCREEN_SETTINGS) {
      settingsSelectedField = -1;
      lastSettingsField = -99;
    }
  }
}

void DisplayManager::nextScreen() {
  currentScreenIndex = (currentScreenIndex + 1) % totalScreens;
  settingsBgDrawn = false;
  drawImageRegionRGB565_FAST(0, 0, bgScreens[currentScreenIndex], 320, 0, 0, 320, 240);
  clearLastText();
  lastMinText[0] = '\0';
  lastMaxText[0] = '\0';
  if (currentScreenIndex != SCREEN_SETTINGS) {
    settingsSelectedField = -1;
    lastSettingsField = -99;
  }
}

int DisplayManager::getScreenIndex() {
  return currentScreenIndex;
}

Adafruit_ILI9341* DisplayManager::getTFT() {
  return &tft;
}

void DisplayManager::redrawProgressArea(int cx, int cy, int radius) {
  const uint16_t* bg = bgScreens[currentScreenIndex];
  int imgWidth = 320;
  int16_t x = max(0, cx - radius);
  int16_t y = max(0, cy - radius);
  int regionW = min(imgWidth - x, radius * 2);
  int regionH = min(240 - y, radius * 2);
  drawImageRegionRGB565_FAST(x, y, bg, imgWidth, x, y, regionW, regionH);
}

void DisplayManager::clearLastText() {
  lastTime[0] = '\0';
  lastSpeed[0] = '\0';
  lastTemp[0] = '\0';
  lastDriver[0] = '\0';
  lastLicense[0] = '\0';
  lastDrive1[0] = '\0';
  lastDrive2[0] = '\0';
  lastEngine[0] = '\0';
  lastMinText[0] = '\0';
  lastMaxText[0] = '\0';
}

void DisplayManager::clearTextField(char* lastText, int x, int y, const GFXfont* font,
                                    const uint16_t* bgImage, int bgImageWidth) {
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
    lastText[0] = '\0';
  }
}

// ================ Settings ====================
void DisplayManager::showSettingsScreenOnce() {
  if (!settingsBgDrawn) {
    drawImageRegionRGB565_FAST(0, 0, bgScreens[SCREEN_SETTINGS], 320, 0, 0, 320, 240);
    settingsBgDrawn = true;

    // In tiêu đề cố định
    tft.setFont(&FreeSansBoldOblique9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(40, 60);
    tft.print("CAI DAT NHIET DO");
  }
}

void DisplayManager::displaySettings(int tempMin, int tempMax, int selectedField) {
  // Lưu selectedField vào biến trong class (nguồn sự thật)
  settingsSelectedField = selectedField;

  showSettingsScreenOnce();

  // Nếu selection thay đổi so với lần trước -> bcuộ redraw cả hai dòng (xóa vùng và reset lastText)
  if (settingsSelectedField != lastSettingsField) {
    // Xoá vùng chứa MIN và MAX để không để sót chữ/màu cũ
    drawImageRegionRGB565_FAST(50, 95, bgScreens[SCREEN_SETTINGS], 320, 50, 95, 250, 22);  // vùng MIN (rộng hơn để an toàn)
    drawImageRegionRGB565_FAST(50, 145, bgScreens[SCREEN_SETTINGS], 320, 50, 145, 250, 22); // vùng MAX
    lastMinText[0] = '\0';
    lastMaxText[0] = '\0';
    lastSettingsField = settingsSelectedField;
  }

  // Vẽ MIN (màu đỏ nếu selected, cyan nếu không)
  char minBuf[32];
  snprintf(minBuf, sizeof(minBuf), "MIN: %d ", tempMin);
  uint16_t minColor = (settingsSelectedField == 0) ? ILI9341_RED : ILI9341_CYAN;
  displayUpdatedTextFontSmart(minBuf, lastMinText, 70, 100, &FreeSans9pt7b, minColor, bgScreens[SCREEN_SETTINGS], 320);

  // Vẽ MAX
  char maxBuf[32];
  snprintf(maxBuf, sizeof(maxBuf), "MAX: %d ", tempMax);
  uint16_t maxColor = (settingsSelectedField == 1) ? ILI9341_RED : ILI9341_CYAN;
  displayUpdatedTextFontSmart(maxBuf, lastMaxText, 70, 150, &FreeSans9pt7b, maxColor, bgScreens[SCREEN_SETTINGS], 320);
}

// ================ Settings selection accessors ================
void DisplayManager::setSettingsSelectedField(int f) {
  // lưu giá trị; displaySettings sẽ kiểm tra và redraw khi cần
  settingsSelectedField = f;
}

int DisplayManager::getSettingsSelectedField() {
  return settingsSelectedField;
}
