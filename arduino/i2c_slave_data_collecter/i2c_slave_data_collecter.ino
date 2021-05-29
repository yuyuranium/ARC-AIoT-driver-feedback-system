#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define SLAVE_ADDRESS 0x12
#define SERIAL_BAUD 115200
#define FRACTIONAL_BITS 5
#define DEBOUNCE 200
#define PIN_HIMAX_OUT 2
#define PIN_SD 5
#define N_DATA 3

#define STANDBY -1

const uint8_t N_CLASS = 2;         // number of buttons
const uint8_t BTN_PIN[N_CLASS] = {4, 3};  // moodify later
const uint8_t LED_PIN[N_CLASS] = {7, 8};

// choose the 2 classes we want to label
// const char *CLASS_NAME[N_CLASS] = {"0_idle", "1_cruise"};
const char *CLASS_NAME[N_CLASS] = {"2_start", "3_stop"};
// const char *CLASS_NAME[N_CLASS] = {"4_left", "5_right"};

unsigned long btn_trigger_time[N_CLASS] = {0, 0};

const char *DATA_NAME[N_DATA] = {"ax", "ay", "az"};

int8_t state;

File data_entry;

void setup() {
  state = STANDBY;
  
  // Serial.begin(SERIAL_BAUD);

  while (!SD.begin(PIN_SD)) {
    ;
  }

  delay(500);

  pinMode(PIN_HIMAX_OUT, OUTPUT);
  digitalWrite(PIN_HIMAX_OUT, LOW);

  for (uint8_t i = 0; i < N_CLASS; ++i) {
    Serial.println(CLASS_NAME[i]);
    pinMode(BTN_PIN[i], INPUT_PULLUP);
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
    
    if (! SD.exists(CLASS_NAME[i])) {
      SD.mkdir(CLASS_NAME[i]);
    }
    delay(200);
  }

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
}

void loop() {
  boolean btn_state[N_CLASS];
  for (uint8_t i = 0; i < N_CLASS; ++i) {
    btn_state[i] = digitalRead(BTN_PIN[i]);
  }
  if (state == STANDBY) {
    for (uint8_t i = 0; i < N_CLASS; ++i) {
      if (btn_state[i] == LOW && debounce(i)) {
        int count = 0;
        char filename[80];
        do {
          count++;
          sprintf(filename, "%s/%d.csv", CLASS_NAME[i], count);
        } while (SD.exists(filename));
        data_entry = SD.open(filename, FILE_WRITE);
        for (int j = 0; j < N_DATA - 1; ++j) {
          data_entry.print(DATA_NAME[j]);
          data_entry.print(',');
        }
        data_entry.println(DATA_NAME[N_DATA - 1]);
        // Serial.print("Writing: ");
        // Serial.println(filename);
        state = i;
        digitalWrite(LED_PIN[state], HIGH);
        digitalWrite(PIN_HIMAX_OUT, HIGH);
        break;
      }
    }
  } else {
    if (btn_state[state] == LOW && debounce(state)) {
      digitalWrite(LED_PIN[state], LOW);
      digitalWrite(PIN_HIMAX_OUT, LOW);
      data_entry.flush();
      data_entry.close();
      state = -1;
      // Serial.println("done");
    }
  }
}

void receiveEvent(int count) {
  if (state != STANDBY) {
    int8_t x, y, z;
    x = Wire.read();
    y = Wire.read();
    z = Wire.read();
    char data_buff[80];
    sprintf(data_buff, "%d,%d,%d", x, y, z);
    if (data_entry)
      data_entry.println(data_buff);
  } else {
    while (Wire.available())
      Wire.read();
  }
}

inline float fixed_to_float(int8_t input) {
  return ((float)input / (float)(1 << FRACTIONAL_BITS));
}

boolean debounce(uint8_t btn) {
  unsigned long now = millis();
  if (now - btn_trigger_time[btn] > DEBOUNCE) {
    btn_trigger_time[btn] = now;
    return true;
  } else {
    btn_trigger_time[btn] = now;
    return false;
  }
}
