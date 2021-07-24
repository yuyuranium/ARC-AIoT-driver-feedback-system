#include <LCDSoftI2C.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h> 

#define SLAVE_ADDRESS 0x12
#define L 0
#define R 1
#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

#define kStates 16
#define kMotions 6
#define kGradingStates 6
#define kGradingStateBegin 9
#define kSdPinOut 10
#define kJoyStickPinRx A1
#define kJoyStickPinRy A2
#define kJoyStickPinSw A3
const float kErrorMean[kGradingStates] = {
  0.6943804, 0.40818594, 0.87863789, 0.9565013, 1.8925444, 1.30906186 };
const float kErrorStd[kGradingStates] = {
  0.26959158, 0.34180247, 0.62125748, 0.3286457, 1.32280363, 0.80790203 };
const float kNormErrorScale = 2.5;
const char kMotionSymbols[kMotions] = { '_', '=', '+', '-', '<', '>' };
const char *kStateNames[kStates] = {
  "[Initial]",
  " Idle?",
  " Start-off?",driver
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

const unsigned long kLocalGradingDuration = 3000;  // 3 seconds
const unsigned long kDebounceDuration = 200;       // 200 ms

float error;
int8_t state;
int8_t motion;
int8_t previous_state;
int8_t grading_state;
bool grading, global_grading, showing_tips;

float total_star[kGradingStates] = {0.0};
int total_star_count[kGradingStates] = {0};

unsigned long show_local_grading_tick;
unsigned long joystick_trigger_time;

char local_grading[17];
int8_t global_grading_state;

LCDSoftI2C lcd(0x27, 16, 2);

typedef union {
  float f_val;
  uint8_t bytes[4];
  uint32_t u32_val;
} floatData;

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
  error = 0.0;
  grading = false;
  global_grading = false;
  showing_tips = false;

  while (state == 0 || state == 15)  {
    LCDShowStateAndMsg(state, motion, "Wait until Idle");
  }
}

void loop() {

   unsigned long now = millis();
  
  if (grading) {
    float norm_error = (error - kErrorMean[grading_state])
        / kErrorStd[grading_state];
    float star = 5 - Sigmoid5(kNormErrorScale * norm_error);
    total_star[grading_state] += star;
    ++total_star_count[grading_state];
    int int_buf = (int)(star * 10);
    int int_part = int_buf / 10;
    int frac_part = int_buf % 10;
    sprintf(local_grading, "%s: %d.%d",
            kGradingStateNames[grading_state],
            int_part,
            frac_part);
    error = 0.0;
    grading = false;
    show_local_grading_tick = now;
  }
  
  int input = ReadJoyStick();
  if (global_grading) {
    
    // Enter interactive mode
    if (input != -1 && JoyStickDebounce()) {
      switch (input) {
        case 0:
          global_grading = false;
          break;
        case 1:
          if (global_grading_state < 5 && !showing_tips)
            ++global_grading_state;
          break;
        case 2:
          if (total_star_count[global_grading_state] > 0)
            showing_tips = true;
          break;
        case 3:
          if (global_grading_state > 0 && !showing_tips)
            --global_grading_state;
          break;
        case 4:
          showing_tips = false;
          break; 
      }
    }

    if (showing_tips) {
      LCDShowGlobalGradingTips(global_grading_state);
    } else {
      LCDShowGlobalGrading(global_grading_state);
    }
    return;
  }

  if (input == 0 && JoyStickDebounce()) {
    global_grading = true;
    return;
  }
  
  if (now - show_local_grading_tick < kLocalGradingDuration) {
    LCDShowStateAndMsg(state, motion, local_grading);
  } else if (state == 8) {
    LCDShowStateAndMsg(state, motion, "(P) See details");
  } else {
    LCDShowStateAndMsg(state, motion, "Grading ...");
  }
}

inline float Sigmoid5(const float x) {
  return 5 / (1 + exp(-x));
}

int ReadJoyStick() {
  int sw = digitalRead(kJoyStickPinSw);
  int rx = analogRead(kJoyStickPinRx);
  int ry = analogRead(kJoyStickPinRy);
  if (sw == LOW)
    return 0;
  if (ry > 1000)
    return 1;
  if (rx > 1000)
    return 2;
  if (ry < 20)
    return 3;
  if (rx < 20)
    return 4;
  return -1;
}

void LCDShowGlobalGradingTips(int8_t grading_state) {
  int int_star = round(total_star[grading_state] / total_star_count[grading_state]);
  if (int_star == 5) {
    LCDShowMsg("<Very good!", " Try to keep it");
  } else if (int_star == 0) {
    LCDShowMsg("<You may be", " a Sambao");
  } else if (int_star == 4) {
    LCDShowMsg("<Good! You can", " be better");
  } else if (int_star == 3) {
    LCDShowMsg("<More gentle", " can be better");
  } else if (int_star == 2) {
    LCDShowMsg("<Ctrl thr/brake", " more carefully");
  } else if (int_star == 1) {
    LCDShowMsg("<Try to keep", " vehicle stable");
  } else {
    LCDShowMsg("<Unsual", "score");
  }
}

void LCDShowGlobalGrading(int8_t grading_state) {
  char buf1[17], buf2[17];
  char arrow[3];
  if (grading_state == 0) {
    sprintf(arrow, " D");
  } else if (grading_state == 5) {
    sprintf(arrow, "U ");
  } else {
    sprintf(arrow, "UD");
  }
  if (total_star_count[grading_state] == 0) {
    char grading[17];
    sprintf(grading, "%s: N/A", kGradingStateNames[grading_state]);
    sprintf(buf1, "%-16s", grading);
    sprintf(buf2, "%-13s%2s ", "Total: 0", arrow);
  } else {
    float star = total_star[grading_state] / total_star_count[grading_state];
    int int_buf = (int)(star * 10);
    int int_part = int_buf / 10;
    int frac_part = int_buf % 10;
    char total[15];
    char grading[17];
    sprintf(grading, "%s: %d.%d", kGradingStateNames[grading_state], int_part, frac_part);
    sprintf(buf1, "%-16s", grading);
    sprintf(total, "Total: %d", total_star_count[grading_state]);
    sprintf(buf2, "%-13s%2s>", total, arrow);
  }
  lcd.setCursor(0, 0);
  lcd.print(buf1);
  lcd.setCursor(0, 1);
  lcd.print(buf2);
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

void LCDShowMsg(char *row1, char *row2) {
  char buf1[17], buf2[17];
  sprintf(buf1, "%-16s", row1);
  sprintf(buf2, "%-16s", row2);
  lcd.setCursor(0, 0);
  lcd.print(buf1);
  lcd.setCursor(0, 1);
  lcd.print(buf2);
}

void ReceiveEvent(int count) {
  while (Wire.available()) {
    floatData error_raw;
    error_raw.bytes[0] = Wire.read();
    error_raw.bytes[1] = Wire.read();
    error_raw.bytes[2] = Wire.read();
    error_raw.bytes[3] = Wire.read();
    state = Wire.read();
    motion = Wire.read();
    if (!grading && error_raw.u32_val != 0) {
      error = error_raw.f_val;
      grading_state = previous_state - 9;
      grading = true;
    }
    previous_state = state;
  }
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
