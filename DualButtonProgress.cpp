#include "DualButtonProgress.h"
#include "DisplayManager.h"
#include "MinMaxEEPROM.h"
#include <math.h>

DualButtonProgress::DualButtonProgress(uint8_t btn1, uint8_t btn2, uint8_t btn3, DisplayManager* displayMgr)
  : _btn1(btn1), _btn2(btn2), _btn3(btn3), _displayMgr(displayMgr), _holdTime(1000),
    _startTime(0), _isHolding(false), _completed(false), _lastPercent(-1), _callback(nullptr) {}

void DualButtonProgress::begin() {
  pinMode(_btn1, INPUT_PULLUP);
  pinMode(_btn2, INPUT_PULLUP);
  pinMode(_btn3, INPUT_PULLUP);
}

void DualButtonProgress::setHoldTime(uint16_t ms) {
  _holdTime = ms;
}

void DualButtonProgress::onComplete(void (*callback)()) {
  _callback = callback;
}

void DualButtonProgress::update() {
  int screen = _displayMgr->getScreenIndex();
  if (screen == SCREEN_SETTINGS) {
    handleSettings();
    return;
  }

  bool b1 = (digitalRead(_btn1) == HIGH);
  bool b2 = (digitalRead(_btn2) == HIGH);
  bool b3 = (digitalRead(_btn3) == HIGH);

  // Chuyển màn hình: giữ đồng thời 2 nút bất kỳ
  if ((b1 && b2) || (b1 && b3) || (b2 && b3)) {
    if (!_isHolding) {
      _isHolding = true;
      _completed = false;
      _startTime = millis();
      _lastPercent = -1;
    }

    unsigned long elapsed = millis() - _startTime;
    int percent = min(100, (int)((elapsed * 100) / _holdTime));

    if (percent != _lastPercent) {
      drawProgress(percent);
      _lastPercent = percent;
    }

    if (percent >= 100 && !_completed) {
      _completed = true;
      if (_callback) _callback();
      _displayMgr->redrawProgressArea(160, 120, 34);
      _isHolding = false;
    }
  } else {
    if (_isHolding) {
      _displayMgr->redrawProgressArea(160, 120, 34);
    }
    _isHolding = false;
    _completed = false;
    _lastPercent = -1;
  }
}

// Sửa lại handleSettings:
void DualButtonProgress::handleSettings() {
  bool b1 = (digitalRead(_btn1) == HIGH);  // BTN_OK (GPIO8): chuyển trường
  bool b2 = (digitalRead(_btn2) == HIGH);  // BTN_DOWN (GPIO3): tăng
  bool b3 = (digitalRead(_btn3) == HIGH);  // BTN_UP (GPIO5): giảm

  static uint32_t lastPress = 0;
  static int okPressCount = 0;  // Đếm số lần nhấn BTN_OK liên tiếp

  int field = _settingField;
  int tempMin = _displayMgr->tempMin;
  int tempMax = _displayMgr->tempMax;

  // Chọn trường: BTN_OK
  if (b1 && !b2 && !b3 && millis() - lastPress > 300) {
    okPressCount++;
    lastPress = millis();

    if (okPressCount == 3) {
      okPressCount = 0;
      saveMinMaxToEEPROM(tempMin, tempMax);
      _displayMgr->setSettingsSelectedField(-1);  // thoát selection
      if (_callback) _callback();
      return;
    }
    _settingField = (_settingField + 1) % 2;
    // Thông báo cho DisplayManager biết mục đã thay đổi
    _displayMgr->setSettingsSelectedField(_settingField);
  }
  // Nếu nhấn nút khác thì reset lại đếm
  if ((!b1 && b2) || (!b1 && b3) || (b1 && b2 && b3)) {
    okPressCount = 0;
  }

  // Tăng: BTN_DOWN
  if (!b1 && b2 && !b3 && millis() - lastPress > 300) {
    if (_settingField == 0) tempMin = min(tempMin + 1, min(tempMax - 1, 100));
    else tempMax = min(tempMax + 1, 100);
    lastPress = millis();
  }
  // Giảm: BTN_UP
  if (!b1 && !b2 && b3 && millis() - lastPress > 300) {
    if (_settingField == 0) tempMin = max(-100, tempMin - 1);
    else tempMax = max(tempMax - 1, max(tempMin + 1, -100));
    lastPress = millis();
  }

  // Giới hạn lại để đảm bảo: min ≤ max-1, max ≥ min+1
  tempMin = max(-100, min(tempMin, min(tempMax - 1, 100)));
  tempMax = min(100, max(tempMax, max(tempMin + 1, -100)));

  // Nếu thay đổi, cập nhật giá trị
  if (tempMin != _displayMgr->tempMin || tempMax != _displayMgr->tempMax || field != _settingField) {
    _displayMgr->tempMin = tempMin;
    _displayMgr->tempMax = tempMax;
    _displayMgr->displaySettings(tempMin, tempMax, _settingField);
  }
}
void DualButtonProgress::drawProgress(int percent) {
  Adafruit_ILI9341* _tft = _displayMgr->getTFT();

  int cx = 160, cy = 120, r_outer = 34, r_inner = 30;

  _displayMgr->redrawProgressArea(cx, cy, r_outer);

  float angle_end = (float)percent * 3.6;

  _tft->drawCircle(cx, cy, r_inner, ILI9341_DARKGREY);

  for (int i = 0; i < angle_end; i += 2) {
    float rad = (float)i * 0.0174533;
    int x1 = cx + cos(rad) * r_inner;
    int y1 = cy + sin(rad) * r_inner;
    int x2 = cx + cos(rad) * (r_inner + 4);
    int y2 = cy + sin(rad) * (r_inner + 4);
    _tft->drawLine(x1, y1, x2, y2, ILI9341_GREEN);
  }
}
