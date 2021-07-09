#include <SoftwareSerial.h>

#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

const uint8_t kSeg = 7;
const uint8_t kClass = 6;
const char *kClassName[kClass] = { "IDLE", "CRUISE", "ACCEL", "BRAKE", "LEFT", "RIGHT" };
const uint8_t kSegPinOut[kSeg] = { 8, 9, 3, 2, 4, 6, 5 };  // a, b, c, d, e, f, g
const uint8_t kSegOut[kClass][kSeg] = {
  { 1, 1, 1, 1, 1, 1, 0 },
  { 0, 1, 1, 0, 0, 0, 0 },
  { 1, 1, 0, 1, 1, 0, 1 },
  { 1, 1, 1, 1, 0, 0, 1 },
  { 0, 1, 1, 0, 0, 1, 1 },
  { 1, 0, 1, 1, 0, 1, 1 },
};
const uint8_t kJoyStickPinRx = A1;
const uint8_t kJoyStickPinRy = A0;
const uint8_t kJoyStickPinSw = 12;
const uint8_t kSwitchPin = 10;
const uint8_t kButtonPin = 7;

const unsigned long kDebounceDuration = 200;  // 200 ms
unsigned long sw_trigger_time;
unsigned long btn_trigger_time;

const int upper_thresh = 1000;
const int lower_thresh = 20;

uint8_t roller = 0;

int8_t class_type = 0;
boolean is_ready = false;

SoftwareSerial BTSerial(A2, A3);

void setup() {
  Serial.begin(SERIAL_BAUD);

  // bluetooth setup
  BTSerial.begin(BTSERIAL_BAUD);

  // joystick setup
  pinMode(kJoyStickPinRx, INPUT);
  pinMode(kJoyStickPinRy, INPUT);
  pinMode(kJoyStickPinSw, INPUT);
  digitalWrite(kJoyStickPinSw, HIGH);

  // sw setup
  pinMode(kSwitchPin, INPUT_PULLUP);
  
  // btn setup
  pinMode(kButtonPin, INPUT_PULLUP);
  
  // seven seg setup
  for (int i = 0; i < kSeg; ++i) {
    pinMode(kSegPinOut[i], OUTPUT);
    digitalWrite(kSegPinOut[i], LOW);
  }
}

void loop() {
  boolean set = !digitalRead(kSwitchPin);
  if (set && debounce_sw) {
    int sw = digitalRead(kJoyStickPinSw);
    int rx = analogRead(kJoyStickPinRx);
    int ry = analogRead(kJoyStickPinRy);
    if (sw == LOW) {
      class_type = 0;
    }
    switch (class_type) {
      case -1:
        class_type = (sw)? 1 : 0;
        break;
      case 0:
        if (rx > 1000)
          class_type = 2;
        break;
      case 1:
        // accel and brake has higher priority so need not exam if joystick is centered
        if (rx > 1000) {
          class_type = 2;
        } else if (rx < 20) {
          class_type = 3;
        } else if (rx < 600 && rx > 400) {
          // only when controller is placed middle will it trigger left or right
          if (ry > 1000) {
            class_type = 5;
          } else if (ry < 20) {
            class_type = 4;
          }
        }
        break;
      case 2:
      case 3:
        if (rx < 600 && rx > 400) {
          // only when controller is placed middle will it trigger left or right
          if (ry > 1000) {
            class_type = 5;
          } else if (ry < 20) {
            class_type = 4;
          } else {
            class_type = 1;
          }
        }
        break;
      case 4:
      case 5:
        if (ry < 600 && ry > 400) {
          // only when controller is placed middle will it trigger left or right
          if (rx > 1000) {
            class_type = 2;
          } else if (ry < 20) {
            class_type = 3;
          } else {
            class_type = 1;
          }
        }
      default:
        break;
    }
    for (int i = 0; i < kSeg; ++i) {
      digitalWrite(kSegPinOut[i], kSegOut[class_type][i]);
    }
    BTSerial.print(class_type);
    Serial.println(class_type);
  } else {
    class_type = -1;
    boolean regret = !digitalRead(kButtonPin);
    if (regret && debounce_btn) {
      // on regret
      BTSerial.print('r');
      Serial.println('r');
      for (int i = 0; i < kSeg; ++i) {
        digitalWrite(kSegPinOut[i], LOW);
      }
      // show a dash on seven seg
      digitalWrite(kSegPinOut[kSeg - 1], HIGH);
    } else {
      // standby
      BTSerial.print('s');
      Serial.println('s');
      for (int i = 0; i < kSeg; ++i) {
        digitalWrite(kSegPinOut[i], LOW);
      }
      // roll the seven seg
      digitalWrite(kSegPinOut[roller++], HIGH);
      if (roller == 6)
        roller = 0;
    }
  }
  delay(50);
}

boolean debounce_sw() {
  unsigned long now = millis();
  if (now - sw_trigger_time > kDebounceDuration) {
    sw_trigger_time = now;
    return true;
  } else {
    sw_trigger_time = now;
    return false;
  }
}

boolean debounce_btn() {
  unsigned long now = millis();
  if (now - btn_trigger_time > kDebounceDuration) {
    btn_trigger_time = now;
    return true;
  } else {
    btn_trigger_time = now;
    return false;
  }
}
