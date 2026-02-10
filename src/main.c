#include <stdio.h>                // standard C library (not strictly needed for blinking)
#include <zephyr/kernel.h>        // Zephyr OS: threads + sleeping + printk
#include <zephyr/drivers/gpio.h>  // GPIO driver API (LED pins are GPIO pins)
#include <leds_funcs.h>           // our helper: setup_leds() configures LED pins

/* Time helpers: 1000 msec = 1 second */
#define SLEEP_TIME_MS 1000         // 1000 milliseconds = 1 second

/* Blink timing (milliseconds) */
#define ON_TIME_MS 50              // keep LED ON for 50 ms
#define OFF_TIME_MS 150            // keep LED OFF for 150 ms

/* How many LEDs we plan to use */
#define NUM_LEDS 4                 // we will blink 4 LEDs total
// const size_t NUM_LEDS = 4;  // NUM_LEDS has to be a #define to use it in macros

/* Thread settings */
#define MY_STACK_SIZE 1024         // stack memory for EACH thread
#define MY_PRIORITY 5              // thread priority (all threads use same priority)

/* Devicetree: get the board's LED nodes (led0, led1, led2, led3). */
#define LED0_NODE DT_NODELABEL(led0) 
#define LED1_NODE DT_NODELABEL(led1)  
#define LED2_NODE DT_NODELABEL(led2)  
#define LED3_NODE DT_NODELABEL(led3)  

/*
 * Each thread needs its own stack (scratch space for function calls/locals).
 * Here we create NUM_LEDS stacks, each MY_STACK_SIZE bytes.
 */
K_KERNEL_STACK_ARRAY_DEFINE(thread_stacks, NUM_LEDS, MY_STACK_SIZE);
struct k_thread thread_datas[NUM_LEDS];   // stores thread objects (one per LED)
k_tid_t thread_tids[NUM_LEDS];            // stores thread IDs (one per LED)

/*
 * ELI5 note about "stacks":
 * - Each thread is like a little mini-program running at the same time.
 * - Each mini-program needs its own "scratch paper" for function calls and local variables.
 * - That scratch paper is called the thread's STACK.
 * - So we make an array of stacks: one stack per LED thread.
 */

/*
 * gpio_dt_spec = "GPIO description from devicetree":
 * it tells Zephyr which GPIO controller + which pin + flags.
 */
const struct gpio_dt_spec gds_leds[NUM_LEDS] = {   // actual GPIO info for each LED
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),          // LED0 pin info from devicetree
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),          // LED1 pin info from devicetree
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),          // LED2 pin info from devicetree
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),          // LED3 pin info from devicetree
};

/* Array of pointers so we can easily pass "which LED" around. */
const struct gpio_dt_spec* p_gds_leds[NUM_LEDS] = { // pointers to each LED's gpio_dt_spec
	&gds_leds[0], &gds_leds[1], &gds_leds[2], &gds_leds[3]
};

/* All the arguments one blink thread needs. */
struct blink_args {
	const struct gpio_dt_spec* led;  // which LED this thread controls
	size_t on_time_ms;               // ON time (ms)
	size_t off_time_ms;              // OFF time (ms)
};

/* One argument packet per LED/thread (global so it stays valid forever). */
static struct blink_args blink_params[NUM_LEDS];

/*
 * Thread entry function:
 * Zephyr threads always use the signature: void (*)(void*, void*, void*)
 * We pass a pointer to a blink_args struct as p1.
 */
void blink_single_led(void* p1, void* p2, void* p3)
{
	ARG_UNUSED(p2);  // thread functions accept 3 args; we only use p1
	ARG_UNUSED(p3);  // so we mark p2/p3 unused

	/* Convert the generic void* argument into our real type. */
	const struct blink_args* args = (const struct blink_args*)p1; // "my instructions"

	printk("Blink thread started for LED %p (on=%u ms, off=%u ms)\n",
	       args->led, (unsigned)args->on_time_ms, (unsigned)args->off_time_ms);
	while (true) {                              // loop forever (this thread never ends)
		gpio_pin_set_dt(args->led, 1);          // 1) turn THIS thread's LED ON
		k_msleep(args->on_time_ms);             // 2) wait while it stays ON
		gpio_pin_set_dt(args->led, 0);          // 3) turn THIS thread's LED OFF
		k_msleep(args->off_time_ms);            // 4) wait while it stays OFF
	}
};

int main(void)
{
	/* 1) Configure all LED pins as outputs (start OFF). */
	int ret = setup_leds(p_gds_leds, NUM_LEDS);  // prepare LED GPIO pins so we can use them
	if (ret < 0) {
		return 0;  // if setup fails, stop the program
	}

	printk("Starting threads...\n");
	k_msleep(500);  // small delay before blinking begins

	/* 2) Create one blinking thread per LED. */
	for (size_t idx = 0; idx < NUM_LEDS; idx++) {    // do this for LED 0,1,2,3
		/* Fill in the parameters for LED idx's blink thread. */
		blink_params[idx].led = p_gds_leds[idx];      // pick which LED this thread will control
		blink_params[idx].on_time_ms = ON_TIME_MS;    // store ON time
		blink_params[idx].off_time_ms = OFF_TIME_MS;  // store OFF time

		thread_tids[idx] = k_thread_create(
			&(thread_datas[idx]),                      // thread object storage
			thread_stacks[idx],                        // stack memory for this thread
			K_KERNEL_STACK_SIZEOF(thread_stacks[idx]), // size of that stack
			blink_single_led,                           // thread function to run
			(void*)&blink_params[idx], NULL, NULL,      // p1 points to this thread's args
			MY_PRIORITY,                                // priority
			0,                                          // options (none)
			K_NO_WAIT);                                 // start now (immediately)
	}

	/*
	 * ELI5: Why do all LEDs blink at the same time?
	 * - Because we start 4 threads.
	 * - Each thread runs its own infinite loop.
	 * - Zephyr quickly switches between them, so they all make progress together.
	 * - There is nothing here that says "LED0 must finish first, then LED1, then LED2, then LED3".
	 */

	/* 3) main thread goes idle; blinking happens in the worker threads. */
	while (1) {                      // main thread lives forever too
		k_msleep(SLEEP_TIME_MS);     // it just sleeps; worker threads do the blinking
	}
	return 0;
}
