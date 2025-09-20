#include <Arduino.h>
#include "DisplayManager.h"
#include "DualButtonProgress.h"
#include "PacketParser.h"
#include "voice.h"
#include "MinMaxEEPROM.h"

// ==== Biến toàn cục ====
DisplayManager displayMgr;
DualButtonProgress* dualBtn;
ParsedData g_data;
SemaphoreHandle_t dataMutex;

HardwareSerial RS232Serial(1);  // UART1 cho RS232

// ================= TASKS =================

// Task hiển thị TFT
void displayTask(void* pv) {
  for (;;) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      if (displayMgr.getScreenIndex() == SCREEN_SETTINGS) {
        // lấy mục đang chọn từ DisplayManager (nguồn sự thật)
        int sel = displayMgr.getSettingsSelectedField();
        displayMgr.displaySettings(displayMgr.tempMin, displayMgr.tempMax, sel);
      } else {
        displayMgr.display(g_data);
      }
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(150)); // update nhanh hơn để cảm nhận tức thì khi đổi màu
  }
}

// Task nút nhấn
void buttonTask(void* pv) {
  for (;;) {
    dualBtn->update();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Task âm thanh
void voiceTask(void* pv) {
  for (;;) {
    loopVoiceAlert();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Task cảnh báo lái xe
void drivingAlertTask(void* pv) {
  for (;;) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      if (g_data.speed > 3) {  // xe đang chạy
        if (g_data.continuousDrive >= 240) playVoiceAlertDrive4h();
        else if (g_data.continuousDrive >= 225) playVoiceAlertSoon4h();

        if (g_data.dailyDrive >= 600) playVoiceAlertDrive10h();
        else if (g_data.dailyDrive >= 585) playVoiceAlertSoon10h();
      }
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Task đọc dữ liệu RS232
void rs232Task(void* pv) {
  String packet;
  for (;;) {
    while (RS232Serial.available()) {
      char c = RS232Serial.read();
      if (c == '\n') {
        ParsedData d = parsePacket(packet);
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          g_data = d;
          xSemaphoreGive(dataMutex);
        }
        packet = "";
      } else {
        packet += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);  // debug
  EEPROM.begin(64);

  displayMgr.init();
  setupVoiceAlert();

  // Load min/max từ EEPROM
  loadMinMaxFromEEPROM(displayMgr.tempMin, displayMgr.tempMax);

  // Nút nhấn: BTN_OK=GPIO8, BTN_DOWN=GPIO3, BTN_UP=GPIO5
  dualBtn = new DualButtonProgress(8, 3, 5, &displayMgr);
  dualBtn->begin();
  dualBtn->setHoldTime(1000);
  dualBtn->onComplete([]() {
    displayMgr.nextScreen();
  });

  dataMutex = xSemaphoreCreateMutex();

  // Giá trị mặc định
  g_data.timestamp = "2025-09-19 12:00:00";
  g_data.speed = 0;
  g_data.temperature = 25.0;
  g_data.driverName = "Nguyen";
  g_data.licenseID = "51A-12345";
  g_data.continuousDrive = 0;
  g_data.dailyDrive = 0;
  g_data.engineOn = true;

  // RS232: UART1 (GPIO20 RX, GPIO21 TX) → chỉnh theo phần cứng
  RS232Serial.begin(9600, SERIAL_8N1, 20, 21);

  // ==== Tạo task FreeRTOS ====
  xTaskCreate(displayTask,     "Display", 4096, NULL, 1, NULL);
  xTaskCreate(buttonTask,      "Buttons", 2048, NULL, 1, NULL);
  xTaskCreate(voiceTask,       "Voice",   4096, NULL, 1, NULL);
  xTaskCreate(drivingAlertTask,"Alert",   4096, NULL, 1, NULL);
  xTaskCreate(rs232Task,       "RS232",   4096, NULL, 1, NULL);
}

void loop() {
  // không dùng
}
