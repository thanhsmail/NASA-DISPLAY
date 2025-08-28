#include "voice.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "sound10h.h"  // chứa beep_wav và beep_wav_len
#include "sound4h.h" 
#include "soundOver4h.h"
#include "soundOver10h.h"
static AudioGeneratorWAV *wav = nullptr;
static AudioOutputI2S *out = nullptr;
static AudioFileSourcePROGMEM *file = nullptr;

void setupVoiceAlert() {
  out = new AudioOutputI2S();
  out->SetPinout(0, 1, 2); // BCLK, LRC, DIN
  out->SetGain(1.5);
  wav = new AudioGeneratorWAV();
}

// void playVoiceAlert() {
//   if (!wav || wav->isRunning()) return;
//   file = new AudioFileSourcePROGMEM(beep_wav, beep_wav_len);
//   if (!wav->begin(file, out)) {
//     Serial.println("Lỗi phát âm cảnh báo!");
//   }
// }

void loopVoiceAlert() {
  if (wav && wav->isRunning()) {
    wav->loop();
  }
}
void playVoiceAlertDrive4h() {
  if (!wav || wav->isRunning()) return;
  file = new AudioFileSourcePROGMEM(beep_over_4h, beep_wav_over_4h);
  wav->begin(file, out);
  //Serial.println("Cảnh báo: Đã lái xe quá 4 giờ!");
}

void playVoiceAlertSoon4h() {
  if (!wav || wav->isRunning()) return;
  file = new AudioFileSourcePROGMEM(beep_4h, beep_wav_4h);
  wav->begin(file, out);
  //Serial.println("Cảnh báo: Sắp lái xe quá 4 giờ!");
}

void playVoiceAlertDrive10h() {
  if (!wav || wav->isRunning()) return;
  file = new AudioFileSourcePROGMEM(beep_over_10h, beep_wav_over_10h);
  wav->begin(file, out);
  //Serial.println("Cảnh báo: Đã lái xe quá 10 giờ!");
}

void playVoiceAlertSoon10h() {
  if (!wav || wav->isRunning()) return;
  file = new AudioFileSourcePROGMEM(beep_10h, beep_wav_10h);
  wav->begin(file, out);
  //Serial.println("Cảnh báo: Sắp lái xe quá 10 giờ!");
}

