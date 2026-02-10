#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <leds_funcs.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000
#define ON_TIME_MS 50
#define OFF_TIME_MS 150
#define NUM_LEDS 4				// We have 4 LEDs
// const size_t NUM_LEDS = 4;  // NUM_LEDS has to be a #define to use it in macros

#define MY_STACK_SIZE 1024		// Stack size 1024 bytes
#define MY_PRIORITY 5     	   // Ensures all threads have the same importance (Not sure what 1,2,3,4 do)

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_NODELABEL(led0)
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)
#define LED3_NODE DT_NODELABEL(led3)

// single thread stack: K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
K_KERNEL_STACK_ARRAY_DEFINE(thread_stacks, NUM_LEDS, MY_STACK_SIZE);			//This creates an array of four memory stacks (one for each LED thread)
struct k_thread thread_datas[NUM_LEDS];  // threads must be defined outside of a function
k_tid_t thread_tids[NUM_LEDS];

const struct gpio_dt_spec gds_leds[4] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};

const struct gpio_dt_spec* p_gds_leds[4] = {
	&gds_leds[0], &gds_leds[1], &gds_leds[2], &gds_leds[3]
};

void blink_single_led(const struct gpio_dt_spec* p_gds_led, size_t on_time, size_t off_time){
// void blink_single_led(void* v_p_gds_led, void* v_p_on_time, void* v_p_off_time){
	// cast void pointers to the real types:
	// const struct gpio_dt_spec* p_gds_led = (const struct gpio_dt_spec*)v_p_gds_led;
	// size_t on_time = (size_t)v_p_on_time;
	// size_t off_time = (size_t)v_p_off_time;
	printk("p_gds_led %d, ", p_gds_led);
	printk("on_time %d, ", on_time);
	printk("off_time %d\n", off_time);
	while (true) {
		gpio_pin_set_dt(p_gds_led, 1);
		k_msleep(on_time);
		gpio_pin_set_dt(p_gds_led, 0);
		k_msleep(off_time);
	}
};

int main(void)
{
	int ret = setup_leds(p_gds_leds, NUM_LEDS);
	if (ret < 0) {
		return 0;  // returning from the main() function halts the MCU
	}

	printk("Starting threads...\n");
	k_msleep(500);
	for (size_t idx = 0; idx < NUM_LEDS; idx++) {
		thread_tids[idx] = k_thread_create(
			&(thread_datas[idx]), thread_stacks[idx],
			K_KERNEL_STACK_SIZEOF(thread_stacks[idx]),
			blink_single_led,
			(void*) p_gds_leds[idx], (void*) 150, (void*) 200,
			MY_PRIORITY, 0, K_NO_WAIT);
	}

	while (1) {
		k_msleep(1000);  // main thread now does nothing
	}
	return 0;
}
