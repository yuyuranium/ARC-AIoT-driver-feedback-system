#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define SLAVE_ADDRESS 0x12
#define L 0
#define R 1
#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

const uint8_t kData = 12;  // p_ax, p_ay, p_az, ax, ay, az, error, dx, dy, dz, p_class, state
const uint8_t kLeds = 2;
const uint8_t kHimaxPinOut = 2;
const uint8_t kSdPinOut = 5;
const uint8_t kLedPinOut[kLeds] = { 7, 8 };
const char *kDataName[kData] = {
  "p_ax", "p_ay", "p_az", "ax", "ay", "az", "error", "dx", "dy", "dz", "p_class", "state" };
const char kHexDigits[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
const unsigned long kFileDuration = 20000;  // 20 second

int file_counter;
boolean busy;
char val;
int8_t motion;
int8_t state;
unsigned long last;

SoftwareSerial BTSerial(A2, A3);
File data_entry;

typedef union {
  float f_val;
  uint8_t bytes[4];
} floatData;

void setup() {
  Serial.begin(SERIAL_BAUD);

  // bluetooth setup
  BTSerial.begin(BTSERIAL_BAUD);
  
  // himax eable setup
  pinMode(kHimaxPinOut, OUTPUT);
  digitalWrite(kHimaxPinOut, LOW);

  // i2c setup
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);

  // led setup
  pinMode(kLedPinOut[L], OUTPUT);
  pinMode(kLedPinOut[R], OUTPUT);
  digitalWrite(kLedPinOut[L], LOW);
  digitalWrite(kLedPinOut[R], LOW);

  // SD card setup
  while (!SD.begin(kSdPinOut)) {
    // slowly blink 2 leds to indicate sd card error
    // wait until no error
    digitalWrite(kLedPinOut[L], !digitalRead(kLedPinOut[R]));  // sync with R
    digitalWrite(kLedPinOut[R], !digitalRead(kLedPinOut[R]));
    delay(500);
  }

  delay(200);

  // initialize variables
  busy = false;
  file_counter = 0;
  motion = -1;
  state = 0;
  digitalWrite(kLedPinOut[L], LOW);
  digitalWrite(kLedPinOut[R], LOW);
  char filename[10];
  do {
    sprintf(filename, "p%d.csv", ++file_counter);
  } while (SD.exists(filename));  // initialize file_counter with latest file count
  data_entry = SD.open(filename, FILE_WRITE);
  int i;
  for (i = 0; i < kData - 1; ++i) {
    data_entry.print(kDataName[i]);
    data_entry.print(',');
    Serial.print(kDataName[i]);
    Serial.print(',');
  }
  data_entry.println(kDataName[i]);
  Serial.println(kDataName[i]);
  last = millis();
  busy = true;
}

void loop() {
  if (millis() - last < kFileDuration) return;

  // close the file and open a new one
  busy = false;  // disable writing file
  data_entry.flush();
  data_entry.close();

  char filename[10];
  do {
    sprintf(filename, "p%d.csv", ++file_counter);
  } while (SD.exists(filename));
  data_entry = SD.open(filename, FILE_WRITE);
  int i;
  for (i = 0; i < kData - 1; ++i) {
    data_entry.print(kDataName[i]);
    data_entry.print(',');
    Serial.print(kDataName[i]);
    Serial.print(',');
  }
  data_entry.println(kDataName[i]);
  Serial.println(kDataName[i]);
  last = millis();
  busy = true;
}

void receiveEvent(int count) {
  // one float data has 4 bytes, so there are 'count / 4' float data
  const int n_float_data = count >> 2;
  floatData data[n_float_data];
  for (int i = 0; i < n_float_data; ++i) {
    // for each byte, put them in place and take them out as float
    for (int j = 0; j < 4; ++j) {
      unsigned char b = Wire.read();
      data[i].bytes[j] = b;
    }
  }
  // read the last one byte left as classification index
  if (Wire.available()) {
    motion = Wire.read();
  }
  if (Wire.available()) {
    state = Wire.read();
  }
  // update leds
  switch (motion) {
    case 0:
      // blink 2
      digitalWrite(kLedPinOut[L], !digitalRead(kLedPinOut[R]));  // sync with R
      digitalWrite(kLedPinOut[R], !digitalRead(kLedPinOut[R]));
      break;
    case 1:
      digitalWrite(kLedPinOut[L], LOW);
      digitalWrite(kLedPinOut[R], HIGH);
      break; 
    case 2:
      digitalWrite(kLedPinOut[L], HIGH);
      digitalWrite(kLedPinOut[R], LOW);
      break;
    case 3:
      digitalWrite(kLedPinOut[L], HIGH);
      digitalWrite(kLedPinOut[R], HIGH);
      break;
    case 4:
      // blink L
      digitalWrite(kLedPinOut[L], !digitalRead(kLedPinOut[L]));
      digitalWrite(kLedPinOut[R], LOW);
      break;
    case 5:
      // blink R
      digitalWrite(kLedPinOut[L], LOW);
      digitalWrite(kLedPinOut[R], !digitalRead(kLedPinOut[R]));
      break;
    default:
      digitalWrite(kLedPinOut[L], LOW);
      digitalWrite(kLedPinOut[R], LOW);
      break;
  }
  // if it is to write a file
  if (busy) {
    for (int i = 0; i < n_float_data; ++i) {
      data_entry.print(data[i].f_val, 5);
      data_entry.print(",");
      Serial.print(data[i].f_val, 5);
      Serial.print(",");
    }
    for (int i = 0; i < 3; ++i) {
      float diff = data[i + 3].f_val - data[i].f_val;
      data_entry.print(diff, 5);
      data_entry.print(",");
      Serial.print(diff, 5);
      Serial.print(",");
    }
    data_entry.print(motion);
    data_entry.print(",");
    data_entry.println(state);
    Serial.print(motion);
    Serial.print(",");
    Serial.println(state);
    BTSerial.print(kHexDigits[state]);
  }
}
