#include "hx_drv_tflm.h"
#include "synopsys_wei_delay.h"
#include "synopsys_wei_gpio.h"

#include "stdio.h"
#include "math.h"

#define ARDUINO 0x12

#define FRACTIONAL_BITS 5

int8_t data_buf[3];

// built in led
hx_drv_gpio_config_t hal_led_r;

// gpio to recieve arduino control signal
hx_drv_gpio_config_t hal_gpio_arduino_in;

inline int8_t float_to_fixed(float x) {
  return (int8_t)(round(x * (1 << FRACTIONAL_BITS)));
}

int main(int argc, char *argv[]) {

	hx_drv_uart_initial(UART_BR_115200);

  if (hal_gpio_init(&hal_led_r, HX_DRV_LED_RED, HX_DRV_GPIO_OUTPUT, GPIO_PIN_RESET) == HAL_OK)
    hx_drv_uart_print("LED RED initialized: OK\n");
  else
    hx_drv_uart_print("LED RED initialized: Error\n");

  if (hal_gpio_init(&hal_gpio_arduino_in, HX_DRV_PGPIO_2, HX_DRV_GPIO_INPUT, GPIO_PIN_RESET) == HAL_OK)
    hx_drv_uart_print("GPIO ARDUINO IN initialized: OK\n");
  else
    hx_drv_uart_print("GPIO ARDUINO IN initialized: OK\n");

	if (hx_drv_accelerometer_initial() != HX_DRV_LIB_PASS)
		hx_drv_uart_print("Accelerometer Initialize Fail\n");
	else
		hx_drv_uart_print("Accelerometer Initialize Success\n");

  hal_gpio_set(&hal_led_r, GPIO_PIN_RESET);

  hx_drv_share_switch(SHARE_MODE_I2CM);
	while (1) {
		uint32_t available_count = 0;
		float x, y, z;

		available_count = hx_drv_accelerometer_available_count();				
		for (uint32_t i = 0; i < available_count; i++) {
			hx_drv_accelerometer_receive(&x, &y, &z);
		}

    hx_drv_gpio_get(&hal_gpio_arduino_in);

    if (hal_gpio_arduino_in.gpio_data == 1) {
      data_buf[0] = float_to_fixed(x);
      data_buf[1] = float_to_fixed(y);
      data_buf[2] = float_to_fixed(z);
      hx_drv_i2cm_set_data(ARDUINO, NULL, 0, data_buf, 3);
      hal_led_r.gpio_data = 1 - hal_led_r.gpio_data;
      hx_drv_gpio_set(&hal_led_r);
    } else {
      hal_gpio_set(&hal_led_r, GPIO_PIN_RESET);
    }

		hal_delay_ms(45);
	}
}
