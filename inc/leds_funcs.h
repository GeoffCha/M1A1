#include <stdio.h>
#include <zephyr/drivers/gpio.h>   // Zephyr GPIO types/functions (LED pins are GPIOs)

int setup_leds(const struct gpio_dt_spec* arr_gds_leds[], size_t num_leds) {
	// This function "prepares" a list of LEDs so we can turn them on/off later.
	// arr_gds_leds = array (list) of pointers to LED pin descriptions (from devicetree)
	// num_leds     = how many LEDs are in that list
	// Returns: 0 on success, negative error code on failure.

	int ret;  // store return/error codes from Zephyr GPIO calls

	for (size_t idx = 0; idx < num_leds; idx++) {  // loop over every LED in the list
		// Check the GPIO device behind this LED is ready (drivers initialized, etc.).
		if (!gpio_is_ready_dt(arr_gds_leds[idx])) {
			return -1;  // "not ready" (simple generic error code)
		}

		// Configure the LED pin as an OUTPUT, starting in the OFF state.
		ret = gpio_pin_configure_dt(arr_gds_leds[idx], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {  // negative means an error in Zephyr
			return ret;  // return the specific Zephyr error code
		}
	}

	return 0;  // all LEDs configured successfully
}

void set_leds(const struct gpio_dt_spec* arr_gds_leds[], size_t num_leds, bool IS_ON) {
	// This function turns ALL LEDs in the list ON or OFF.
	//
	// IS_ON = true -> turn on, false -> turn off
	for (size_t idx = 0; idx < num_leds; idx++) {       // for each LED...
		gpio_pin_set_dt(arr_gds_leds[idx], IS_ON);        // set its output level (1=on, 0=off)
	}
}