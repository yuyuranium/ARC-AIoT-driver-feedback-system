#include <Wire.h>
#define SLAVE_ADDRESS 0x12

const uint8_t LED_PIN[2] = {8, 7};

uint8_t index = 0;

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 2; ++i) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
  }

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
}

void loop() {
  Serial.println(index);
  switch (index) {
    case 0:
      Serial.println("0: Idle");
      digitalWrite(LED_PIN[0], LOW);
      digitalWrite(LED_PIN[1], LOW);
      break;
    case 1:
      Serial.println("1: Cruise");
      digitalWrite(LED_PIN[0], HIGH);
      digitalWrite(LED_PIN[1], LOW);
      break;
    case 2:
      Serial.println("2: Start");
      digitalWrite(LED_PIN[0], LOW);
      digitalWrite(LED_PIN[1], HIGH);
      break;
    case 3:
      Serial.println("3: Stop");
      digitalWrite(LED_PIN[0], HIGH);
      digitalWrite(LED_PIN[1], HIGH);
      break;
    default:
      Serial.println("Unknown");
      digitalWrite(LED_PIN[0], LOW);
      digitalWrite(LED_PIN[1], LOW);
      break;
  }
}

void receiveEvent(int count) {
  while (Wire.available()) {
    index = Wire.read();
  }
}
