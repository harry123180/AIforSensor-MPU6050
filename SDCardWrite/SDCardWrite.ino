#include <SPI.h>
#include <SD.h>

#define CS_PIN 9
#define SCK_PIN 10
#define MOSI_PIN 11
#define MISO_PIN 12

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  // 設定自定義SPI接腳
  SPI1.setRX(MISO_PIN);
  SPI1.setTX(MOSI_PIN);
  SPI1.setSCK(SCK_PIN);
  
  Serial.println("初始化SD卡...");
  
  if (!SD.begin(CS_PIN, SPI1)) {
    Serial.println("SD卡初始化失敗");
    return;
  }
  
  Serial.println("SD卡初始化成功");
  
  // 創建並寫入檔案
  File file = SD.open("hello.txt", FILE_WRITE);
  if (file) {
    file.println("hello world");
    file.close();
    Serial.println("檔案寫入完成");
  } else {
    Serial.println("無法開啟檔案");
  }
  
  // 驗證檔案內容
  file = SD.open("hello.txt");
  if (file) {
    Serial.println("檔案內容:");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  }
}

void loop() {
  // 空的主迴圈
}