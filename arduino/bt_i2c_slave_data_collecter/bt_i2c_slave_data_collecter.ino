#include <SoftwareSerial.h>
#include <LCDSoftI2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define SLAVE_ADDRESS 0x12
#define SERIAL_BAUD 115200
#define BTSERIAL_BAUD 38400

const uint8_t kData = 7;  // ax, ay, az, jx, jy, jz, class
const uint8_t kMotions = 6;
const uint8_t kHimaxPinOut = 9;
const uint8_t kSdPinOut = 10;
const unsigned long kDebounceDuration = 2000;  // 2 s
const char *kDataName[kData] = { "ax", "ay", "az", "jx", "jy", "jz", "class" };
const char kMotionSymbols[kMotions] = { '_', '=', '+', '-', '<', '>' };

unsigned long regret_trigger_time;
uint8_t class_type;
int file_counter;
boolean busy;
char val;

SoftwareSerial BTSerial(A2, A1);
LCDSoftI2C lcd(0x27, 16, 2);
File data_entry;

typedef union {
  float f_val;
  uint8_t bytes[sizeof(float)];
} floatData;

void setup() {
  // lcd setup
  lcd.init();
  lcd.backlight();
  LCDShowMsg("BT Data", "Collecter");
  delay(1000);
  LCDShowMsg("By Southerner", "NCKU CASLab");
  delay(1000);
  
  Serial.begin(SERIAL_BAUD);

  
  
  // himax eable setup
  pinMode(kHimaxPinOut, OUTPUT);
  digitalWrite(kHimaxPinOut, LOW);

  // i2c setup
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);

  // SD card setup
  while (!SD.begin(kSdPinOut)) {
    // slowly blink 2 leds to indicate sd card error
    // wait until no error
    LCDShowMsg("SD card", "Init failed");
    delay(500);
  }

  delay(200);

  // initialize variables
  busy = false;
  class_type = -1;
  file_counter = 0;
  char filename[10];
  do {
    sprintf(filename, "%d.csv", ++file_counter);
  } while (SD.exists(filename));  // initialize file_counter with latest file count
  LCDShowMsg("Label begin:", filename);
  delay(1000);
  regret_trigger_time = millis();

  // bluetooth setup
  BTSerial.begin(BTSERIAL_BAUD);
}

void loop() {
  if (BTSerial.available()) {
    val = BTSerial.read();
    if (busy) {
      if (val == 's') {
        // disable himax, blow out the leds, write file and change state
        digitalWrite(kHimaxPinOut, LOW);
        data_entry.flush();
        data_entry.close();
        class_type = -1;
        busy = false;
        Serial.println("Done");
        // LCDShowMsg("Writing", "Done");
      } else if (val <= '5' && val >= '0') {
        // reading class type, convert char to int
        class_type = val - '0';
        lcd.setCursor(0, 1);
        lcd.print("Labeling: ");
        lcd.print(class_type);
      }
    } else {
      if (val == 's') {
        busy = false;  // remain the same
        LCDShowMsg("[P]", "Pending ...");
      } else if (val == 'r' && debounce_regret()) {
        // to avoid double clicking
        // on regret, find the file and delete it
        char filename[10], buf[17];
        sprintf(filename, "%d.csv", file_counter - 1);
        lcd.setCursor(0, 0);
        lcd.print("[R]: ");
        lcd.print(filename);
        if (SD.exists(filename) && SD.remove(filename)) {
          Serial.print("Deleting: ");
          Serial.println(filename);
          --file_counter;
          lcd.setCursor(0, 1);
          lcd.println("Done");
          delay(1000);
          // lit the led for 1 sec to indicate job done

        } else {
          // fast blink 2 times to indicate error
          lcd.setCursor(0, 1);
          lcd.println("Failed        ");
          delay(1000);
        }
      } else if (val <= '5' && val >= '0') {
        char filename[10];
        do {
          sprintf(filename, "%d.csv", file_counter++);
        } while (SD.exists(filename));
        data_entry = SD.open(filename, FILE_WRITE);
        lcd.setCursor(0, 0);
        lcd.print("[W]: ");
        lcd.print(filename);
        int i;
        for (i = 0; i < kData - 1; ++i) {
          data_entry.print(kDataName[i]);
          data_entry.print(',');
        }
        data_entry.println(kDataName[i]);
        class_type = val - '0';
        busy = true;
        digitalWrite(kHimaxPinOut, HIGH);
      }
    }
  }
}

void receiveEvent(int count) {
  if (busy) {
    // one float data has 4 bytes, so there are 'count / 4' float data
    for (int i = 0; i < (count >> 2); ++i) {
      floatData data;
      // for each byte, put them is place and take them out as float
      for (int j = 0; j < 4; ++j) {
        unsigned char b = Wire.read();
        data.bytes[j] = b;
      }
      Serial.print(data.f_val, 5);
      data_entry.print(data.f_val, 5);
      Serial.print(",");
      data_entry.print(",");
    }
    Serial.println(class_type);
    data_entry.println(class_type);
  } else {
    // this should not happen, but just consume all unnecessary data in case it fails
    while (Wire.available())
      Wire.read();
  }
}

// to avoid double regret too quickly
boolean debounce_regret() {
  unsigned long now = millis();
  if (now - regret_trigger_time > kDebounceDuration) {
    regret_trigger_time = now;
    return true;
  } else {
    regret_trigger_time = now;
    return false;
  }
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
