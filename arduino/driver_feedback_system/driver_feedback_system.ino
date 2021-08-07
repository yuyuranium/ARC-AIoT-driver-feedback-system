#include <LCDSoftI2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define SLAVE_ADDRESS 0x12
#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

#define kInstGradingDuration 3000  // 3 seconds
#define kDebounceDuration 600      // 600 ms

#define P 0
#define U 1
#define R 2
#define D 3
#define L 4

const uint8_t kFractionalBits = 8;
const float kQuantScale = (float)(1 << kFractionalBits);

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
const char *kCommentAxisDiscriptions[3] = {
  "@Pavement",
  "@Steering",
  "@Accel-Decel"
};
const char *kIdleMsg = "(P) See details";
const char *kGradingMsg = "Grading ...";

int8_t state;
int8_t motion;
int8_t previous_state;
int8_t previous_motion;
bool state_or_motion_changed;

uint8_t mode;  // To determine what to show on LCD
#define PENDING_MODE 0
#define DRIVING_MODE 1
#define REVIEWING_MODE 2
#define COMMENT_MODE 3

uint8_t reviewing_state;
#define START_OFF 0
#define BRAKE 1
#define LEFT 2
#define RIGHT 3
#define CRUISE 4
#define ACCEL 5
#define RESET 6
#define STORE 7

uint8_t comment_axis;
#define X 0
#define Y 1
#define Z 2
bool show_comment;

char inst_grading_msg[17];
char arrow[5] = "    ";

uint32_t total_gradings[kGradingStates][3] = {0};
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
    // Wait until no error
    LCDShowMsg("Initializing", "SD card...");
    delay(500);
  }

  delay(200);

  // initialize variables
  state = 0;
  motion = -1;
  previous_state = 0;
  previous_motion = -1;
  mode = PENDING_MODE;
  reviewing_state = START_OFF;
  comment_axis = X;
  show_comment = false;
}

void loop() {
  unsigned long now = millis();
  int8_t op = ReadJoyStick();

  switch (mode) {
    case PENDING_MODE: {
      // If system stuck in Initial then reset Himax is needed
      if (state == 0) {
        LCDShowStateAndMsg(state, motion, "Pls reset Himax");
        return;
      }
      
      // Exit condition: Not Initial nor Pending
      if (state != 15) {
        mode = DRIVING_MODE;
        return; 
      }

      // If things changed, update it
      if (state_or_motion_changed) {
        LCDShowStateAndMsg(state, motion, "Wait until Idle");
        return;
      }
      return;
    }
    
    case DRIVING_MODE: {
      // User pressed the joystick
      if (op == P) {
        mode = REVIEWING_MODE;
        LCDShowTotalGrading(reviewing_state);
        return;
      }

      // Got grading
      if (now - show_inst_grading_tick < kInstGradingDuration) {
        if (state_or_motion_changed) {
          LCDShowStateAndMsg(state, motion, inst_grading_msg);
        }
        return;
      }

      if (state == 8) {
        LCDShowStateAndMsg(state, motion, kIdleMsg);
      } else if (state_or_motion_changed) {
        LCDShowStateAndMsg(state, motion, kGradingMsg);
      }
      return;
    }
    
    case REVIEWING_MODE: {
      switch (op) {
        case P:
          mode = DRIVING_MODE;
          if (now - show_inst_grading_tick < kInstGradingDuration) {
            LCDShowStateAndMsg(state, motion, inst_grading_msg);
          } else if (state == 8) {
            LCDShowStateAndMsg(state, motion, kIdleMsg);
          } else {
            LCDShowStateAndMsg(state, motion, kGradingMsg);
          }
          return;

        // U or D is to change state to review
        case U:
          if (reviewing_state > START_OFF)
            --reviewing_state;
          LCDShowTotalGrading(reviewing_state);
          return;
          
        case D:
          if (reviewing_state < ACCEL)
            ++reviewing_state;
          LCDShowTotalGrading(reviewing_state);
          return;

        // R is to go for details of the state, if there exists grading
        case R:
          if (total_grading_counts[reviewing_state]) {
            mode = COMMENT_MODE;
            comment_axis = X;
            LCDShowAxisGrading(reviewing_state, comment_axis);
          }
          return;
      }
      return;
    }
    
    case COMMENT_MODE: {
      if (show_comment) {
        if (op == P) {
          mode = DRIVING_MODE;
          if (now - show_inst_grading_tick < kInstGradingDuration) {
            LCDShowStateAndMsg(state, motion, inst_grading_msg);
          } else if (state == 8) {
            LCDShowStateAndMsg(state, motion, kIdleMsg);
          } else {
            LCDShowStateAndMsg(state, motion, kGradingMsg);
          }
        } else if (op == L) {
          show_comment = false;
          LCDShowAxisGrading(reviewing_state, comment_axis);
        }
        return;
      }
      
      switch (op) {
        case P:
          mode = DRIVING_MODE;
          if (now - show_inst_grading_tick < kInstGradingDuration) {
            LCDShowStateAndMsg(state, motion, inst_grading_msg);
          } else if (state == 8) {
            LCDShowStateAndMsg(state, motion, kIdleMsg);
          } else {
            LCDShowStateAndMsg(state, motion, kGradingMsg);
          }
          return;
          
        case L:
          mode = REVIEWING_MODE;
          LCDShowTotalGrading(reviewing_state);
          return;
        
        case U:
          if (comment_axis > X)
            --comment_axis;
          LCDShowAxisGrading(reviewing_state, comment_axis);
          return;
          
        case D:
          if (comment_axis < Z)
            ++comment_axis;
          LCDShowAxisGrading(reviewing_state, comment_axis);
          return;
          
        case R:
          show_comment = true;
          LCDShowAxisComment(reviewing_state, comment_axis);
          return;
      }
      return;
    }
  }
}

void LCDShowAxisComment(uint8_t reviewing_state, uint8_t comment_axis) {
  char comment_path[20], line1[17], line2[17];
  uint8_t sel = 0;
  uint8_t star10 = (uint8_t)round(FixedToFloat(
      total_gradings[reviewing_state][comment_axis]
      / total_grading_counts[reviewing_state]) * 50);
  if (star10 >= 45) {
    LCDShowMsg("<Excellent!", " Try to keep it");
    return;
  } else if (star10 >= 35) {
    sel = 4;
  } else if (star10 >= 25) {
    sel = 3;
  } else if (star10 >= 0) {
    sel = 2;
  }
  sprintf(comment_path, "COMMENTS/%d%d%d.TXT",
          reviewing_state, sel, comment_axis);
  File comment_file = SD.open(comment_path);
  comment_file.read(line1, 16);
  comment_file.read(line2, 16);
  comment_file.close();
  line1[16] = 0;
  line2[16] = 0;
  LCDShowMsg(line1, line2);
}

void LCDShowAxisGrading(uint8_t reviewing_state, uint8_t comment_axis) {
  char grading_msg[17];
  ResetArrow();
  arrow[0] = '<';
  arrow[3] = '>';
  // Can go up
  if (comment_axis != X)
    arrow[1] = 'U';
  // Can go down
  if (comment_axis != Z)
    arrow[2] = 'D';
  uint8_t star10 = (uint8_t)round(FixedToFloat(
      total_gradings[reviewing_state][comment_axis]
      / total_grading_counts[reviewing_state]) * 50);
  uint8_t i_part = star10 / 10;
  uint8_t f_part = star10 % 10;
  sprintf(grading_msg, "Grade: %d.%d", i_part, f_part);
  LCDShowMsg(kCommentAxisDiscriptions[comment_axis], grading_msg);
  ShowArrow();
}

void LCDShowTotalGrading(uint8_t reviewing_state) {
  char grading_msg[17], grading_count_msg[17];
  ResetArrow();
  // Can go up
  if (reviewing_state != START_OFF)
    arrow[1] = 'U';
  // Can go down
  if (reviewing_state != ACCEL)
    arrow[2] = 'D';
  // If there exists grading for the state, then it can go right to
  // comment mode and need to calculate the average of the grading
  if (total_grading_counts[reviewing_state]) {
    arrow[3] = '>';
    uint8_t star10[3], avg_star10 = 0, min_index = 0;
    for (uint8_t i = 0; i < 3; ++i) {
      star10[i] = (uint8_t)round(FixedToFloat(
          total_gradings[reviewing_state][i]
          / total_grading_counts[reviewing_state]) * 50);
      if (star10[i] < star10[min_index]) {
        min_index = i;
      }
      avg_star10 += (star10[i] >> 2);
    }
    avg_star10 += (star10[min_index] >> 2);
    uint8_t i_part = avg_star10 / 10;
    uint8_t f_part = avg_star10 % 10;
    sprintf(grading_msg, "%s: %d.%d",
            kGradingStateNames[reviewing_state], i_part, f_part);
  } else {
    sprintf(grading_msg, "%s: N/A", kGradingStateNames[reviewing_state]);
  }
  sprintf(grading_count_msg, "Total: %d",
          total_grading_counts[reviewing_state]);
  LCDShowMsg(grading_msg, grading_count_msg);
  ShowArrow();
}

void LCDShowStateAndMsg(int8_t state, int8_t motion, char *msg) {
  char state_and_motion[17];
  char motion_symbol;
  if (motion == -1)
    motion_symbol = '.';
  else
    motion_symbol = kMotionSymbols[motion];
  sprintf(state_and_motion, "%-14s%c ", kStateNames[state], motion_symbol);
  LCDShowMsg(state_and_motion, msg);
}

inline void ShowArrow() {
  lcd.setCursor(12, 1);
  lcd.print(arrow);
}

inline void ResetArrow() {
  for (uint8_t i = 0; i < 4; ++i) {
    arrow[i] = ' ';
  }
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

int8_t ReadJoyStick() {
  int sw = digitalRead(kJoyStickPinSw);
  int rx = analogRead(kJoyStickPinRx);
  int ry = analogRead(kJoyStickPinRy);
  if (sw == LOW && JoyStickDebounce())
    return P;
  if (ry > 800 && JoyStickDebounce())
    return D;
  if (rx > 800 && JoyStickDebounce())
    return R;
  if (ry < 200 && JoyStickDebounce())
    return U;
  if (rx < 200 && JoyStickDebounce())
    return L;
  return -1;
}

void ReceiveEvent(int count) {
  // Retrieve first 2 bytes for state and motion
  if (count >= 2) {
    state = Wire.read();
    motion = Wire.read();
  }

  // If there is more, then it must be the grading of 3 axes
  if (count == 5) {
    uint8_t grading[3], min_index = 0, inst_grading = 0;
    uint8_t grading_state = previous_state - kGradingStateBegin;
    for (uint8_t i = 0; i < 3; ++i) {
      grading[i] = (uint8_t)Wire.read();
      if (grading[i] < grading[min_index]) {
        min_index = i;
      }
      total_gradings[grading_state][i] += grading[i];
      inst_grading += (grading[i] >> 2);
    }
    inst_grading += (grading[min_index] >> 2);
    ++total_grading_counts[grading_state];
    uint8_t star10 = (uint8_t)round(FixedToFloat(inst_grading) * 50);
    uint8_t i_part = star10 / 10;
    uint8_t f_part = star10 % 10;
    sprintf(inst_grading_msg, "%s: %d.%d",
            kGradingStateNames[grading_state], i_part, f_part);
    show_inst_grading_tick = millis();
    state_or_motion_changed = true;
  }

  if (state != previous_state || motion != previous_motion) {
    previous_state = state;
    previous_motion = motion;
    state_or_motion_changed = true;
  } else {
    state_or_motion_changed = false;
  }
  
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
    return false;
  }
}
