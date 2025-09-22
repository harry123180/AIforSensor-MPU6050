#include <Button2.h>

#define BTN1_PIN 4  // GP4
#define BTN2_PIN 5  // GP5

Button2 btn1;
Button2 btn2;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Button2 Multi-Function Example");
  Serial.println("================================");
  
  // 初始化按鈕1
  btn1.begin(BTN1_PIN);
  btn1.setDebounceTime(50);
  btn1.setLongClickTime(1000);
  btn1.setDoubleClickTime(400);
  
  // 按鈕1事件處理
  btn1.setClickHandler([](Button2 &b) {
    Serial.println("BTN1: Single Click");
  });
  
  btn1.setDoubleClickHandler([](Button2 &b) {
    Serial.println("BTN1: Double Click");
  });
  
  btn1.setTripleClickHandler([](Button2 &b) {
    Serial.println("BTN1: Triple Click");
  });
  
  btn1.setLongClickHandler([](Button2 &b) {
    Serial.println("BTN1: Long Press Detected");
  });
  
  // 初始化按鈕2
  btn2.begin(BTN2_PIN);
  btn2.setDebounceTime(50);
  btn2.setLongClickTime(1000);
  btn2.setDoubleClickTime(400);
  
  // 按鈕2事件處理
  btn2.setClickHandler([](Button2 &b) {
    Serial.println("BTN2: Single Click");
  });
  
  btn2.setDoubleClickHandler([](Button2 &b) {
    Serial.println("BTN2: Double Click");
  });
  
  btn2.setTripleClickHandler([](Button2 &b) {
    Serial.println("BTN2: Triple Click");
  });
  
  btn2.setLongClickHandler([](Button2 &b) {
    Serial.println("BTN2: Long Press Detected");
  });
  
  Serial.println("Ready! Press buttons to test:");
  Serial.println("- Single click");
  Serial.println("- Double click");
  Serial.println("- Triple click");
  Serial.println("- Long press (hold > 1 second)");
  Serial.println("================================");
}

void loop() {
  btn1.loop();
  btn2.loop();
}