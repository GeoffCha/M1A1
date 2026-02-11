#include <stdio.h>
#include <zephyr/kernel.h>        // Zephyr threads, sleep, printk, etc.
#include <zephyr/drivers/gpio.h>  // Zephyr GPIO types/functions for LEDs
#include <zephyr/sys/util.h>      // ARG_UNUSED, ARRAY_SIZE
#include <leds_funcs.h>           // our helper functions: setup_leds(), set_leds()

/*
 * Morse code timing uses a base unit "T".
 * - dot:  ON 1T
 * - dash: ON 3T
 * - between dot/dash in same letter: OFF 1T
 * - between words (repeat gap):      OFF 7T
 */
#define T_MS 150

/* How many LEDs we plan to use */
#define NUM_LEDS 4
// const size_t NUM_LEDS = 4;  // NUM_LEDS has to be a #define to use it in macros

/* Thread settings */
#define MY_STACK_SIZE 1024
#define MY_PRIORITY 5

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
struct k_thread thread_datas[NUM_LEDS];  // per-thread bookkeeping (must live long enough)
k_tid_t thread_tids[NUM_LEDS];           // thread IDs returned by k_thread_create()

/*
 * gpio_dt_spec = "GPIO description from devicetree":
 * it tells Zephyr which GPIO controller + which pin + flags.
 */
const struct gpio_dt_spec gds_leds[NUM_LEDS] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};

/* Array of pointers so we can easily pass "which LED" around. */
const struct gpio_dt_spec* p_gds_leds[NUM_LEDS] = {
	&gds_leds[0], &gds_leds[1], &gds_leds[2], &gds_leds[3]
};

/* We are going to keep this SUPER simple:
 * - no structs
 * - no args->something
 * - just 4 thread functions (one per LED/word)
 */

/* Helper: LED ON for ms milliseconds */
static void led_on_for(const struct gpio_dt_spec* led, uint32_t ms)
{
	gpio_pin_set_dt(led, 1);
	k_msleep(ms);
}

/* Helper: LED OFF for ms milliseconds */
static void led_off_for(const struct gpio_dt_spec* led, uint32_t ms)
{
	gpio_pin_set_dt(led, 0);
	k_msleep(ms);
}

/*
 * Blink one letter pattern.
 * pattern is a string like ".-.." or "--."
 */
static void blink_pattern(const struct gpio_dt_spec* led, const char* pattern)
{
	for (size_t i = 0; pattern[i] != '\0'; i++) {
		/* 1) Turn LED on for dot or dash */
		if (pattern[i] == '.') {
			led_on_for(led, 1U * T_MS);
		} else if (pattern[i] == '-') {
			led_on_for(led, 3U * T_MS);
		}

		/* 2) Gap between symbols inside the same letter (OFF 1T) */
		if (pattern[i + 1] != '\0') {
			led_off_for(led, 1U * T_MS);
		}
	}
}

/* Blink a whole word given as an array of letter-pattern strings. */
static void blink_word(const struct gpio_dt_spec* led, const char* const letters[], size_t num_letters)
{
	for (size_t i = 0; i < num_letters; i++) {
		blink_pattern(led, letters[i]);

		/* Gap between letters (OFF 3T), but not after the last letter */
		if (i + 1 < num_letters) {
			led_off_for(led, 3U * T_MS);
		}
	}

	/* Gap between repeats of the word (OFF 7T) */
	led_off_for(led, 7U * T_MS);
}

/*
 * HARD-CODED MORSE WORDS (brute force on purpose)
 *
 * geoff  = g e o f f = --.  .  ---  ..-.  ..-.
 * cha    = c h a     = -.-. .... .-
 * is     = i s       = ..   ...
 * dumb   = d u m b   = -..  ..-  --  -...
 */
static void blink_geoff(const struct gpio_dt_spec* led)
{
	static const char* const letters[] = {"--.", ".", "---", "..-.", "..-."};
	blink_word(led, letters, ARRAY_SIZE(letters));
}

static void blink_cha(const struct gpio_dt_spec* led)
{
	static const char* const letters[] = {"-.-.", "....", ".-"};
	blink_word(led, letters, ARRAY_SIZE(letters));
}

static void blink_is(const struct gpio_dt_spec* led)
{
	static const char* const letters[] = {"..", "..."};
	blink_word(led, letters, ARRAY_SIZE(letters));
}

static void blink_dumb(const struct gpio_dt_spec* led)
{
	static const char* const letters[] = {"-..", "..-", "--", "-..."};
	blink_word(led, letters, ARRAY_SIZE(letters));
}

/* Thread for LED0: blink "geoff" forever */
static void thread_led0(void* p1, void* p2, void* p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Thread LED0 started (geoff)\n");
	while (1) {
		blink_geoff(p_gds_leds[0]);
	}
};

/* Thread for LED1: blink "cha" forever */
static void thread_led1(void* p1, void* p2, void* p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Thread LED1 started (cha)\n");
	while (1) {
		blink_cha(p_gds_leds[1]);
	}
}

/* Thread for LED2: blink "is" forever */
static void thread_led2(void* p1, void* p2, void* p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Thread LED2 started (is)\n");
	while (1) {
		blink_is(p_gds_leds[2]);
	}
}

/* Thread for LED3: blink "dumb" forever */
static void thread_led3(void* p1, void* p2, void* p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Thread LED3 started (dumb)\n");
	while (1) {
		blink_dumb(p_gds_leds[3]);
	}
}

int main(void)
{
	/* 1) Configure all LED pins as outputs (start OFF). */
	int ret = setup_leds(p_gds_leds, NUM_LEDS);
	if (ret < 0) {
		return 0;  // returning from main() stops the program on the MCU
	}

	printk("Starting threads...\n");
	k_msleep(500);

	/* 2) Create one Morse thread per LED. */
	thread_tids[0] = k_thread_create(&(thread_datas[0]), thread_stacks[0],
	                                K_KERNEL_STACK_SIZEOF(thread_stacks[0]),
	                                thread_led0, NULL, NULL, NULL,
	                                MY_PRIORITY, 0, K_NO_WAIT);

	thread_tids[1] = k_thread_create(&(thread_datas[1]), thread_stacks[1],
	                                K_KERNEL_STACK_SIZEOF(thread_stacks[1]),
	                                thread_led1, NULL, NULL, NULL,
	                                MY_PRIORITY, 0, K_NO_WAIT);

	thread_tids[2] = k_thread_create(&(thread_datas[2]), thread_stacks[2],
	                                K_KERNEL_STACK_SIZEOF(thread_stacks[2]),
	                                thread_led2, NULL, NULL, NULL,
	                                MY_PRIORITY, 0, K_NO_WAIT);

	thread_tids[3] = k_thread_create(&(thread_datas[3]), thread_stacks[3],
	                                K_KERNEL_STACK_SIZEOF(thread_stacks[3]),
	                                thread_led3, NULL, NULL, NULL,
	                                MY_PRIORITY, 0, K_NO_WAIT);

	/* 3) main thread goes idle; blinking happens in the worker threads. */
	while (1) {
		k_msleep(1000);
	}
	return 0;
}
