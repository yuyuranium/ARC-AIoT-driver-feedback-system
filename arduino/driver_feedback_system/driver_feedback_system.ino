#include <LCDSoftI2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define SLAVE_ADDRESS 0x12
#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

#define kInstGradingDuration 3000  // 3 seconds
#define kDebounceDuration 200      // 200 ms

const uint8_t kFractionalBits = 5;
const float kQuantScale = (float)(1 << kFractionalBits);

const uint8_t P = 0;
const uint8_t U = 1;
const uint8_t R = 2;
const uint8_t D = 3;
const uint8_t L = 4;

const uint8_t kStates = 16;
const uint8_t kMotions = 6;
const uint8_t kGradingStates = 6;
const uint8_t kGradingStateBegin = 9;
const uint8_t kSdPinOut = 10;
const uint8_t kJoyStickPinRx = A1;
const uint8_t kJoyStickPinRy = A2;
const uint8_t kJoyStickPinSw = A3;

const char kMotionSymbols[kMotions] = { '_', '=', '+', '-', '<', '>' };
const char *kStateNames[kStates] = {
  "[Initial]",
  " Idle?",
  " Start-off?",
  " Brake?",
  " Left?",
  " Right?",
  " Cruise?",
  " Accel?",
  "[Idle]",
  "[Start-off]",
  "[Brake]",
  "[Left]",
  "[Right]",
  "[Cruise]",
  "[Accel]",
  "[Pending]"
};
const char *kGradingStateNames[kGradingStates] = {
  "$Start-off",
  "$Brake",
  "$Left",
  "$Right",
  "$Cruise",
  "$Accel"
};

int8_t state;
int8_t motion;
int8_t previous_state;

char inst_grading_msg[17];

uint16_t total_gradings[kGradingStates][3] = {0};
uint8_t total_grading_counts[kGradingStates] = {0};

unsigned long show_inst_grading_tick;
unsigned long joystick_trigger_time;

LCDSoftI2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(SERIAL_BAUD);

  // lcd setup
  lcd.init();
  lcd.backlight();
  LCDShowMsg("Driver Feedback", "System");
  delay(1000);
  LCDShowMsg("By Southerner", "NCKU CASLab");
  delay(1000);

  // joystick setup
  pinMode(kJoyStickPinRx, INPUT);
  pinMode(kJoyStickPinRy, INPUT);
  pinMode(kJoyStickPinSw, INPUT);
  digitalWrite(kJoyStickPinSw, HIGH);

  // i2c setup
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(ReceiveEvent);

  // SD card setup
  while (!SD.begin(kSdPinOut)) {
    // slowly blink 2 leds to indicate sd card error
    // wait until no error
    LCDShowMsg("Initializing", "SD card...");
  }

  delay(200);

  // initialize variables
  state = 0;
}

void loop() {
  unsigned long now = millis();
  if (now - show_inst_grading_tick < kInstGradingDuration) {
    LCDShowStateAndMsg(state, motion, inst_grading_msg);
  } else if (state == 8) {
    LCDShowStateAndMsg(state, motion, "(P) See details");
  } else {
    LCDShowStateAndMsg(state, motion, "Grading ...");
  }
}

void LCDShowStateAndMsg(int8_t state, int8_t motion, char *msg) {
  char buf1[17], buf2[17];
  char motion_symbol;
  if (motion == -1)
    motion_symbol = '.';
  else
    motion_symbol = kMotionSymbols[motion];
  sprintf(buf1, "%-14s%-2c", kStateNames[state], motion_symbol);
  sprintf(buf2, "%-16s", msg);
  lcd.setCursor(0, 0);
  lcd.print(buf1);
  lcd.setCursor(0, 1);
  lcd.print(buf2);
}

uint8_t ReadJoyStick() {
  int sw = digitalRead(kJoyStickPinSw);
  int rx = analogRead(kJoyStickPinRx);
  int ry = analogRead(kJoyStickPinRy);
  if (sw == LOW)
    return P;
  if (ry > 1000)
    return D;
  if (rx > 1000)
    return R;
  if (ry < 20)
    return U;
  if (rx < 20)
    return L;
  return -1;
}

void ReceiveEvent(int count) {
  // Retrieve first 2 bytes
  if (count >= 2) {
    state = Wire.read();
    motion = Wire.read();
  }

  // If there is more, then it must be the grading of 3 axes
  if (count == 5) {
    uint8_t grading[3], min_index = 0, inst_grading = 0;
    for (int i = 0; i < 3; ++i) {
      grading[i] = (uint8_t)Wire.read();
      if (grading[i] < grading[min_index]) {
        min_index = i;
      }
      total_gradings[previous_state][i] += grading[i];
      inst_grading += grading[i] >> 2;
    }
    inst_grading += grading[min_index] >> 2;
    uint8_t star = (uint8_t)(FixedToFloat(inst_grading) * 10);
    Serial.println(star);
    uint8_t i_part = star / 10;
    uint8_t f_part = star % 10;
    sprintf(inst_grading_msg, "%s: %d.%d",
            kGradingStateNames[previous_state - kGradingStateBegin],
            i_part, f_part);
    show_inst_grading_tick = millis();
  }

  previous_state = state;
}

void LCDShowMsg(char *row1, char *row2) {
  const char *format = "%-16s";
  char buf[17];
  sprintf(buf, format, row1);
  lcd.setCursor(0, 0);
  lcd.print(buf);
  sprintf(buf, format, row2);
  lcd.setCursor(0, 1);
  lcd.print(buf);
}

inline float FixedToFloat(uint8_t f) {
  return (float)f / kQuantScale;
}

bool JoyStickDebounce() {
  unsigned long now = millis();
  if (now - joystick_trigger_time > kDebounceDuration) {
    joystick_trigger_time = now;
    return true;
  } else {
    joystick_trigger_time = now;
    return false;
  }
}
