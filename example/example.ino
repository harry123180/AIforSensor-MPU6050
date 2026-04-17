#include <Button2.h>

// 根據專案慣例定義腳位
#define BTN1_PIN 4  // 按鈕1連接到 GP4
#define R_PIN    0  // 紅色LED連接到 GP0
#define G_PIN    1  // 綠色LED連接到 GP1
#define B_PIN    2  // 藍色LED連接到 GP2

// 建立Button2物件
Button2 btn1;

// 按鈕點擊事件的處理函式
void handleButtonClick(Button2 &b) {
  Serial.println("Button 1 Clicked! Starting RGB sequence.");

  // 亮紅色燈2秒
  digitalWrite(R_PIN, HIGH);
  digitalWrite(G_PIN, LOW);
  digitalWrite(B_PIN, LOW);
  delay(2000);

  // 亮綠色燈2秒
  digitalWrite(R_PIN, LOW);
  digitalWrite(G_PIN, HIGH);
  digitalWrite(B_PIN, LOW);
  delay(2000);

  // 亮藍色燈2秒
  digitalWrite(R_PIN, LOW);
  digitalWrite(G_PIN, LOW);
  digitalWrite(B_PIN, HIGH);
  delay(2000);

  // 關閉所有燈
  digitalWrite(R_PIN, LOW);
  digitalWrite(G_PIN, LOW);
  digitalWrite(B_PIN, LOW);
  Serial.println("RGB sequence finished.");
}

void setup() {
  // 初始化序列埠通訊
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // 等待序列埠連接
  }

  // 設定LED腳位為輸出模式
  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);

  // 初始關閉所有LED
  digitalWrite(R_PIN, LOW);
  digitalWrite(G_PIN, LOW);
  digitalWrite(B_PIN, LOW);

  // 初始化按鈕
  btn1.begin(BTN1_PIN);
  // 設定點擊事件的處理函式
  btn1.setClickHandler(handleButtonClick);

  Serial.println("System Ready. Press BTN1 to start.");
}

void loop() {
  // 持續偵測按鈕狀態
  btn1.loop();
}
