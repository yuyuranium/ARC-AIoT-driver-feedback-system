#include "hx_drv_tflm.h"
#include "synopsys_wei_gpio.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#define ARDUINO 0x12
#define X 0
#define Y 1
#define Z 2

const float kLPFBeta = 0.25;
const float kSamplingPeriod = 0.04;
const uint32_t kClkRate = 400000000;
const uint32_t kSamplingCycle = kSamplingPeriod*kClkRate;
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
} floatData;

uint8_t string_buf[128] = "test\n";

int main(int argc, char *argv[]) {
  int32_t int_buf;
  float raw_ax, raw_ay, raw_az;
  float accel_last[3];
  floatData accel[3];
  floatData jerk[3];
  uint32_t tick_now = 0, tick_last = 0;

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

  for (int i = 0; i < 3; ++i) {
    jerk[i].f_val = 0.0;
    accel[i].f_val = 0.0;
    accel_last[i] = 0.0;
  }

  while (1) {
    uint32_t available_count = 0;

    // recording new accel data as well as flushing unused
    // always read the latest data
    available_count = hx_drv_accelerometer_available_count();       
    for (uint32_t i = 0; i < available_count; ++i) {
      hx_drv_accelerometer_receive(&raw_ax, &raw_ay, &raw_az);
    }
    hx_drv_tick_get(&tick_now);  // always read the latest cc

    hx_drv_gpio_get(&hal_gpio_arduino_in);
    if (hal_gpio_arduino_in.gpio_data == 1) {
      // to keep the same sampling rate, i.e., the same dt
      if (tick_now - tick_last < kSamplingCycle)
        continue;  // wait until dt is long enough

      // not until we have long enough dt will the control flow goes here

      // LPF: Y(n) = (1-ß)*Y(n-1) + ß*X(n) = Y(n-1) - ß(Y(n-1)-X(n));
      accel[X].f_val = accel[X].f_val - (kLPFBeta * (accel[X].f_val - raw_ax));
      accel[Y].f_val = accel[Y].f_val - (kLPFBeta * (accel[Y].f_val - raw_ay));
      accel[Z].f_val = accel[Z].f_val - (kLPFBeta * (accel[Z].f_val - raw_az));

      // calculating jerk of x, y, z
      float dt = (float)(tick_now - tick_last) / (float)kClkRate;
      jerk[X].f_val = (accel[X].f_val - accel_last[X]) / dt;
      jerk[Y].f_val = (accel[Y].f_val - accel_last[Y]) / dt;
      jerk[Z].f_val = (accel[Z].f_val - accel_last[Z]) / dt;

      // store the current data for next cycle
      tick_last = tick_now;
      accel_last[X] = accel[X].f_val;
      accel_last[Y] = accel[Y].f_val;
      accel_last[Z] = accel[Z].f_val;

      for (int i = 0; i < 3; ++i) {
        data_buf[i * 4 + 0] = accel[i].bytes[0];
        data_buf[i * 4 + 1] = accel[i].bytes[1];
        data_buf[i * 4 + 2] = accel[i].bytes[2];
        data_buf[i * 4 + 3] = accel[i].bytes[3];
      }
      for (int i = 3; i < 6; ++i) {
        data_buf[i * 4 + 0] = jerk[i - 3].bytes[0];
        data_buf[i * 4 + 1] = jerk[i - 3].bytes[1];
        data_buf[i * 4 + 2] = jerk[i - 3].bytes[2];
        data_buf[i * 4 + 3] = jerk[i - 3].bytes[3];
      }

      hx_drv_i2cm_set_data(ARDUINO, NULL, 0, data_buf, kDataBytes);
      hal_led_r.gpio_data = !hal_led_r.gpio_data;
      hx_drv_gpio_set(&hal_led_r);
    } else {
      // consume useless data
      accel_last[X] = raw_ax;
      accel_last[Y] = raw_ay;
      accel_last[Z] = raw_az;
      tick_last = tick_now;
      hal_gpio_set(&hal_led_r, GPIO_PIN_RESET);
    }
  }
}
