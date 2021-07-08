#include "hx_drv_tflm.h"
#include "synopsys_wei_delay.h"
#include "synopsys_wei_gpio.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#define ARDUINO 0x12
#define X 0
#define Y 1
#define Z 2

const float kSamplingPeriod = 0.1;
const uint32_t kClkRate = 400000000;
const uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
const uint8_t kAxes = 3;
const uint8_t kDataHimax = 6;

int8_t *data_buf;
const size_t kDataBytes = kDataHimax * sizeof(float);

// built in led
hx_drv_gpio_config_t hal_led_r;

// gpio to recieve arduino control signal
hx_drv_gpio_config_t hal_gpio_arduino_in;

typedef union {
  float f_val;
  uint8_t bytes[sizeof(float)];
} ff;

uint8_t string_buf[128] = "test\n";

int main(int argc, char *argv[]) {
  int32_t int_buf;
  float *accel, *accel_last, *jerk;
  uint32_t tick_start = 0, tick_end = 0;
  accel = malloc(kAxes * sizeof(float));
  accel_last = malloc(kAxes * sizeof(float));
  jerk = malloc(kAxes * sizeof(float));
  data_buf = malloc(kDataBytes);
 
  hx_drv_uart_initial(UART_BR_115200);

  if (hal_gpio_init(&hal_led_r, HX_DRV_LED_RED, HX_DRV_GPIO_OUTPUT,
                    GPIO_PIN_RESET) == HAL_OK)
    hx_drv_uart_print("LED RED initialized: OK\n");
  else
    hx_drv_uart_print("LED RED initialized: Error\n");

  if (hal_gpio_init(&hal_gpio_arduino_in, HX_DRV_PGPIO_2, HX_DRV_GPIO_INPUT,
                    GPIO_PIN_RESET) == HAL_OK)
    hx_drv_uart_print("GPIO ARDUINO IN initialized: OK\n");
  else
    hx_drv_uart_print("GPIO ARDUINO IN initialized: OK\n");

  if (hx_drv_accelerometer_initial() == HX_DRV_LIB_PASS)
    hx_drv_uart_print("Accelerometer Initialize Success\n");
  else
    hx_drv_uart_print("Accelerometer Initialize Fail\n");

  if (hx_drv_tick_start() == HX_DRV_LIB_PASS)
    hx_drv_uart_print("Timer Initialize Success\n");
  else
    hx_drv_uart_print("Timer Initialize Fail\n");
  hal_gpio_set(&hal_led_r, GPIO_PIN_RESET);

  hx_drv_share_switch(SHARE_MODE_I2CM);

  while (1) {
    uint32_t available_count = 0;

    // recording new accel data as well as flushing unused
    // always read the latest data
    available_count = hx_drv_accelerometer_available_count();       
    for (uint32_t i = 0; i < available_count; ++i) {
      hx_drv_accelerometer_receive(&accel[X], &accel[Y], &accel[Z]);
    }
    hx_drv_tick_get(&tick_start);  // always read the latest cc

    hx_drv_gpio_get(&hal_gpio_arduino_in);
    if (hal_gpio_arduino_in.gpio_data == 1) {
      // to keep the same sampling rate, i.e., the same dt
      if (tick_start - tick_end < kSamplingCycle)
        continue;  // wait until dt is long enough

      // not until we have long enough dt will the control flow goes here
      // calculating jerk of x, y, z
      float dt = (float)(tick_start - tick_end) / (float)kClkRate;
      jerk[X] = (accel[X] - accel_last[X]) / dt;
      jerk[Y] = (accel[Y] - accel_last[Y]) / dt;
      jerk[Z] = (accel[Z] - accel_last[Z]) / dt;
      tick_end = tick_start;
      accel_last[X] = accel[X];
      accel_last[Y] = accel[Y];
      accel_last[Z] = accel[Z];

      // split accel float data to 4 bytes each
      ff f;
      for (int a = 0; a < kAxes; ++a) {
        f.f_val = accel[a];
        uint8_t accel_base = a << 2;
        for (int b = 0; b < sizeof(float); ++b) {
          data_buf[accel_base + b] = f.bytes[b];
        }
        f.f_val = jerk[a];
        uint8_t jerk_base = (kAxes + a) << 2;
        for (int b = 0; b < sizeof(float); ++b) {
          data_buf[jerk_base + b] = f.bytes[b];
        }
      }

      hx_drv_i2cm_set_data(ARDUINO, NULL, 0, data_buf, kDataBytes);
      hal_led_r.gpio_data = !hal_led_r.gpio_data;
      hx_drv_gpio_set(&hal_led_r);
    } else {
      // consume useless data
      accel_last[X] = accel[X];
      accel_last[Y] = accel[Y];
      accel_last[Z] = accel[Z];
      tick_end = tick_start;
      hal_gpio_set(&hal_led_r, GPIO_PIN_RESET);
    }
  }
}
