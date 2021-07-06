#include <SoftwareSerial.h>

const int kSerialBuad = 115200;
const int kBTSerialBuad = 38400;
const uint8_t kSeg = 7;
const uint8_t kClass = 6;
const char *kClassName[kClass] = { "IDLE", "CRUISE", "ACCEL", "BRAKE", "LEFT", "RIGHT" };
const uint8_t kSegPinOut[kSeg] = { 3, 2, 6, 7, 8, 4, 5 };  // a, b, c, d, e, f, g
const uint8_t kSegOut[kClass][kSeg] = {
  { 1, 1, 1, 1, 1, 1, 0 },
  { 0, 1, 1, 0, 0, 0, 0 },
  { 1, 1, 0, 1, 1, 0, 1 },
  { 1, 1, 1, 1, 0, 0, 1 },
  { 0, 1, 1, 0, 0, 1, 1 },
  { 1, 0, 1, 1, 0, 1, 1 },
};
const uint8_t kJoyStickPinRx = A3;
const uint8_t kJoyStickPinRy = A2;
const uint8_t kJoyStickPinSw = A1;
const uint8_t kSwitchPin = 9;
const uint8_t kButtonPin = A0;

const unsigned long kDebounceDuration = 200;  // 200 ms
unsigned long sw_trigger_time;
unsigned long btn_trigger_time;

const int upper_thresh = 1000;
const int lower_thresh = 20;

uint8_t roller = 0;

SoftwareSerial BTSerial(11, 12);

void setup() {
  Serial.begin(kSerialBuad);

  // bluetooth setup
  BTSerial.begin(kBTSerialBuad);

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
    int type = 0;
    if (sw) {
      if (rx > upper_thresh) {
        type = 5;
      } else if (rx < lower_thresh) {
        type = 4;
      } else if (ry > upper_thresh) {
        type = 3;
      } else if (ry < lower_thresh) {
        type = 2;
      } else {
        type = 1;
      }
    } else {
      type = 0;
    }
    for (int i = 0; i < kSeg; ++i) {
      digitalWrite(kSegPinOut[i], kSegOut[type][i]);
    }
    BTSerial.print(type);
    Serial.println(type);
  } else {
    boolean regret = !digitalRead(kButtonPin);
    if (regret && debounce_btn) {
      BTSerial.print('r');
      Serial.println('r');
      for (int i = 0; i < kSeg; ++i) {
        digitalWrite(kSegPinOut[i], LOW);
      }
      digitalWrite(kSegPinOut[kSeg - 1], HIGH);
      delay(50);
    } else {
      BTSerial.print('s');
      Serial.println('s');
      for (int i = 0; i < kSeg; ++i) {
        digitalWrite(kSegPinOut[i], LOW);
      }
      digitalWrite(kSegPinOut[roller++], HIGH);
      if (roller == 6)
        roller = 0;
      delay(50);
    }
  }
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
