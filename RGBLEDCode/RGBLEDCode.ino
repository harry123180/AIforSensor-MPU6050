#define R 0
#define G 1
#define B 2

void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
pinMode(R,OUTPUT);
pinMode(G,OUTPUT);
pinMode(B,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(R,1);
  delay(100);
  digitalWrite(R,0);
  delay(100);
  digitalWrite(G,1);
  delay(100);
  digitalWrite(G,0);
  delay(100);
  digitalWrite(B,1);
  delay(100);
  digitalWrite(B,0);
  delay(100);
}
