/*
 * main_morse_test.c
 *
 * EASIEST / MOST READABLE Morse example (ELI5 style)
 *
 * Goal:
 * - LED0 blinks "Geoff"
 * - LED1 blinks "Chavez"
 * - LED2 blinks "likes"
 * - LED3 blinks "Digimon"
 *
 * And we do it SEQUENTIALLY (one after the other):
 *   LED0 word -> then LED1 word -> then LED2 word -> then LED3 word -> repeat forever
 *
 * No threads in this file (on purpose) because this is the simplest to understand.
 */

#include <zephyr/kernel.h>        // k_msleep(), printk()
#include <zephyr/drivers/gpio.h>  // GPIO control for LEDs
#include <leds_funcs.h>           // setup_leds()

/* Morse timing uses a base unit "T" (milliseconds). */
#define T_MS 150

/* Morse rules (International Morse):
 * - DOT  = LED ON 1T
 * - DASH = LED ON 3T
 * - gap between dot/dash within a letter = LED OFF 1T
 * - gap between words = LED OFF 7T
 */

#define NUM_LEDS 4

/* Get the board's LED nodes from devicetree. */
#define LED0_NODE DT_NODELABEL(led0)
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)
#define LED3_NODE DT_NODELABEL(led3)

/* Turn devicetree LED nodes into real GPIO pin descriptions. */
static const struct gpio_dt_spec gds_leds[NUM_LEDS] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};

static const struct gpio_dt_spec* p_leds[NUM_LEDS] = {
	&gds_leds[0], &gds_leds[1], &gds_leds[2], &gds_leds[3]
};

/* --- Tiny LED helpers (so the Morse code reads nicely) --- */

static void led_on(const struct gpio_dt_spec* led)  { gpio_pin_set_dt(led, 1); }
static void led_off(const struct gpio_dt_spec* led) { gpio_pin_set_dt(led, 0); }

static void sleep_ms(uint32_t ms) { k_msleep(ms); }

/* Morse ON pieces */
static void dot(const struct gpio_dt_spec* led)  { led_on(led); sleep_ms(1U * T_MS); led_off(led); }
static void dash(const struct gpio_dt_spec* led) { led_on(led); sleep_ms(3U * T_MS); led_off(led); }

/* Morse OFF gaps */
static void gap_1T(const struct gpio_dt_spec* led) { led_off(led); sleep_ms(1U * T_MS); }
static void gap_3T(const struct gpio_dt_spec* led) { led_off(led); sleep_ms(3U * T_MS); }
static void gap_7T(const struct gpio_dt_spec* led) { led_off(led); sleep_ms(7U * T_MS); }

/*
 * Blink one letter given as a pattern string:
 *   "."  means dot
 *   "-"  means dash
 * Example: 'c' is "-.-."
 */
static void blink_letter(const struct gpio_dt_spec* led, const char* pattern)
{
	for (size_t i = 0; pattern[i] != '\0'; i++) {
		if (pattern[i] == '.') {
			dot(led);
		} else if (pattern[i] == '-') {
			dash(led);
		}

		/* If there is another symbol coming, add the 1T symbol gap */
		if (pattern[i + 1] != '\0') {
			gap_1T(led);
		}
	}
}

/* After a letter finishes, we want a 3T gap before the next letter. */
static void letter_gap(const struct gpio_dt_spec* led) { gap_3T(led); }

/* After a word finishes, we want a 7T gap before repeating / next word. */
static void word_gap(const struct gpio_dt_spec* led) { gap_7T(led); }

/* --- Word functions (hard-coded so it's easy to read) --- */

/* geoff = g e o f f */
static void blink_geoff(const struct gpio_dt_spec* led)
{
	/* g = --. */
	blink_letter(led, "--."); letter_gap(led);
	/* e = . */
	blink_letter(led, ".");   letter_gap(led);
	/* o = --- */
	blink_letter(led, "---"); letter_gap(led);
	/* f = ..-. */
	blink_letter(led, "..-."); letter_gap(led);
	/* f = ..-. */
	blink_letter(led, "..-.");

	word_gap(led);
}

/* chavez = c h a v e z */
static void blink_chavez(const struct gpio_dt_spec* led)
{
	blink_letter(led, "-.-."); letter_gap(led);  /* c */
	blink_letter(led, "...."); letter_gap(led);  /* h */
	blink_letter(led, ".-");   letter_gap(led);  /* a */
	blink_letter(led, "...-"); letter_gap(led);  /* v */
	blink_letter(led, ".");    letter_gap(led);  /* e */
	blink_letter(led, "--..");                 /* z */

	word_gap(led);
}

/* likes = l i k e s */
static void blink_likes(const struct gpio_dt_spec* led)
{
	blink_letter(led, ".-.."); letter_gap(led);  /* l */
	blink_letter(led, "..");   letter_gap(led);  /* i */
	blink_letter(led, "-.-");  letter_gap(led);  /* k */
	blink_letter(led, ".");    letter_gap(led);  /* e */
	blink_letter(led, "...");                 /* s */

	word_gap(led);
}

/* digimon = d i g i m o n */
static void blink_digimon(const struct gpio_dt_spec* led)
{
	blink_letter(led, "-.."); letter_gap(led);   /* d */
	blink_letter(led, "..");  letter_gap(led);   /* i */
	blink_letter(led, "--."); letter_gap(led);   /* g */
	blink_letter(led, "..");  letter_gap(led);   /* i */
	blink_letter(led, "--");  letter_gap(led);   /* m */
	blink_letter(led, "---"); letter_gap(led);   /* o */
	blink_letter(led, "-.");                  /* n */

	word_gap(led);
}

int main(void)
{
	/* 1) Make sure LED GPIO pins are ready and configured as outputs. */
	int ret = setup_leds(p_leds, NUM_LEDS);
	if (ret < 0) {
		return 0;  // stop if LEDs can't be used
	}

	printk("Morse test: sequential words on LEDs\n");
	k_msleep(500);

	/* 2) Loop forever. One LED at a time (sequential). */
	while (1) {
		blink_geoff(p_leds[0]);    // LED0 says "Geoff"
		blink_chavez(p_leds[1]);   // LED1 says "Chavez"
		blink_likes(p_leds[2]);    // LED2 says "likes"
		blink_digimon(p_leds[3]);  // LED3 says "Digimon"
	}
}

