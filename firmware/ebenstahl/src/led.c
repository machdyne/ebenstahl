/*
 * Ebenstahl LED
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * The LED reflects two independent pieces of state -- the USB link and the
 * SCSI medium -- plus two momentary overlays (activity and fault). led_task()
 * renders whichever of these has the highest priority:
 *
 *   1. fault      red flash            (rapid blink on single-LED boards)
 *   2. suspended  off
 *   3. unmounted  green blink
 *   4. ejected    steady blue          (short blink every 2s on single-LED)
 *   5. activity   green, full brightness
 *   6. idle       green, breathing
 *
 * Brightness is PWM'd for two reasons: the LEDs are bright enough to be
 * distracting at full duty, and the idle breathe needs a smooth ramp.
 */

#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "ebenstahl.h"
#include "led.h"

// 1023 @ default clkdiv puts the carrier at ~122kHz, far above visible
// flicker, and 10 bits keeps resolution where it matters: the bottom of
// the curve, where a dimmed breathe spends most of its time.
#define LED_PWM_TOP				1023

/*
 * Per-channel brightness ceilings, in perceptual units (0-255).
 *
 * All three channels share a 536R resistor but the LED forward voltages
 * differ a lot: red is roughly (3.3-2.0)/536 = 2.4mA while green and blue
 * sit near 3.0V Vf and draw only ~0.6mA. Red is therefore about 4x the
 * current of the other two, and a single global cap would leave it glaring
 * while green stayed dim. These are estimates from the schematic, not
 * measured -- tune them against real hardware.
 */
#define LED_TRIM_R				70
#define LED_TRIM_G				160
#define LED_TRIM_B				180
#define LED_TRIM_MONO			160

#define LED_ACTIVITY_MS			40		// activity stretch, see led_note_activity()
#define LED_BREATHE_MS			4000	// full breathe cycle
#define LED_BREATHE_MIN			60		// idle floor; never fully extinguishes
#define LED_BREATHE_MAX			180		// stays well under the activity level, so a
										// breath peak can't be mistaken for a transfer
#define LED_BLINK_UNMOUNTED_MS	250
#define LED_EJECT_PERIOD_MS		2000
#define LED_EJECT_BLINK_MS		120

#ifdef ES_HAS_RGB_LED
#define LED_FAULT_MS			400		// single red flash
#else
#define LED_FAULT_MS			1000	// rapid blink burst
#define LED_FAULT_BLINK_MS		100
#endif

static led_link_t led_link = LED_LINK_UNMOUNTED;
static led_medium_t led_medium = LED_MEDIUM_PRESENT;

static bool led_activity_pending = false;
static uint32_t led_activity_ts = 0;

static bool led_fault_pending = false;
static uint32_t led_fault_ts = 0;

static uint32_t led_epoch = 0;

static inline uint32_t led_now_ms(void) {
	return to_ms_since_boot(get_absolute_time());
}

/*
 * Perceptual brightness (0-255) to PWM level.
 *
 * A linear duty ramp looks like it snaps bright early and then plateaus, so
 * the breathe needs gamma correction to look natural. b*b >> 6 is gamma 2.0
 * in integer arithmetic and tops out at 1016, just under LED_PWM_TOP.
 */
static inline uint16_t led_gamma(uint8_t b) {
	return (uint16_t)(((uint32_t)b * (uint32_t)b) >> 6);
}

static inline void led_put(uint gpio, uint8_t value, uint8_t trim) {
	uint8_t scaled = (uint8_t)(((uint32_t)value * (uint32_t)trim) / 255);
	pwm_set_gpio_level(gpio, led_gamma(scaled));
}

#ifdef ES_HAS_RGB_LED
static void led_rgb(uint8_t r, uint8_t g, uint8_t b) {
	led_put(ES_LEDR, r, LED_TRIM_R);
	led_put(ES_LEDG, g, LED_TRIM_G);
	led_put(ES_LEDB, b, LED_TRIM_B);
}
#else
static void led_mono(uint8_t v) {
	led_put(ES_LED, v, LED_TRIM_MONO);
}
#endif

static void led_pwm_init_pin(uint gpio) {

	gpio_set_function(gpio, GPIO_FUNC_PWM);

	uint slice = pwm_gpio_to_slice_num(gpio);
	uint chan = pwm_gpio_to_channel(gpio);

	pwm_set_wrap(slice, LED_PWM_TOP);

#if ES_LED_ACTIVE_LOW
	// the LEDs are common anode, so invert in hardware rather than writing
	// (TOP - level) at every call site
	pwm_set_output_polarity(slice, chan == PWM_CHAN_A, chan == PWM_CHAN_B);
#endif

	pwm_set_chan_level(slice, chan, 0);
	pwm_set_enabled(slice, true);

}

void led_init(void) {

#ifdef ES_HAS_RGB_LED
	// R, G and B land on separate PWM slices (GPIO n -> slice (n>>1)&7), so
	// each channel gets an independent duty with no shared-counter conflict
	led_pwm_init_pin(ES_LEDR);
	led_pwm_init_pin(ES_LEDG);
	led_pwm_init_pin(ES_LEDB);
	led_rgb(0, 0, 0);
#else
	led_pwm_init_pin(ES_LED);
	led_mono(0);
#endif

	led_epoch = led_now_ms();

}

void led_set_link(led_link_t link) {
	led_link = link;
}

void led_set_medium(led_medium_t medium) {
	led_medium = medium;
}

// A single 512-byte SPI transfer finishes in well under a millisecond, so
// bursts are stretched to LED_ACTIVITY_MS to make them visible. Sustained
// transfers hold the LED at full brightness.
void led_note_activity(void) {
	led_activity_ts = led_now_ms();
	led_activity_pending = true;
}

void led_note_fault(void) {
	led_fault_ts = led_now_ms();
	led_fault_pending = true;
}

// triangle ramp through the gamma curve, LED_BREATHE_MIN..LED_BREATHE_MAX
static uint8_t led_breathe_level(uint32_t now) {

	uint32_t phase = (uint32_t)(now - led_epoch) % LED_BREATHE_MS;
	uint32_t half = LED_BREATHE_MS / 2;

	uint32_t tri = (phase < half) ?
	    (phase * 255u / half) : ((LED_BREATHE_MS - phase) * 255u / half);

	if (tri > 255u) tri = 255u;

	return (uint8_t)(LED_BREATHE_MIN +
	    ((tri * (LED_BREATHE_MAX - LED_BREATHE_MIN)) / 255u));

}

void led_task(void) {

	uint32_t now = led_now_ms();

	// 1. fault
	if (led_fault_pending) {
		if ((uint32_t)(now - led_fault_ts) < LED_FAULT_MS) {
#ifdef ES_HAS_RGB_LED
			led_rgb(255, 0, 0);
#else
			bool on = (((uint32_t)(now - led_fault_ts) /
			    LED_FAULT_BLINK_MS) & 1u) == 0u;
			led_mono(on ? 255 : 0);
#endif
			return;
		}
		led_fault_pending = false;
	}

	// 2. suspended: USB requires < 2.5mA average from a bus-powered device
	// in suspend, so the LED goes dark rather than blinking
	if (led_link == LED_LINK_SUSPENDED) {
#ifdef ES_HAS_RGB_LED
		led_rgb(0, 0, 0);
#else
		led_mono(0);
#endif
		return;
	}

	// 3. waiting for enumeration
	if (led_link == LED_LINK_UNMOUNTED) {
		bool on = ((now / LED_BLINK_UNMOUNTED_MS) & 1u) != 0u;
#ifdef ES_HAS_RGB_LED
		led_rgb(0, on ? 200 : 0, 0);
#else
		led_mono(on ? 200 : 0);
#endif
		return;
	}

	// 4. medium ejected: steady, so it can't be mistaken for the idle breathe
	if (led_medium == LED_MEDIUM_EJECTED) {
#ifdef ES_HAS_RGB_LED
		led_rgb(0, 0, 200);
#else
		bool on = (now % LED_EJECT_PERIOD_MS) < LED_EJECT_BLINK_MS;
		led_mono(on ? 200 : 0);
#endif
		return;
	}

	// 5. activity
	if (led_activity_pending) {
		if ((uint32_t)(now - led_activity_ts) < LED_ACTIVITY_MS) {
#ifdef ES_HAS_RGB_LED
			led_rgb(0, 255, 0);
#else
			led_mono(255);
#endif
			return;
		}
		led_activity_pending = false;
	}

	// 6. mounted and idle
	uint8_t b = led_breathe_level(now);
#ifdef ES_HAS_RGB_LED
	led_rgb(0, b, 0);
#else
	led_mono(b);
#endif

}
