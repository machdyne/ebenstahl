/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2025 Lone Dynamics Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "tusb.h"

#include "ebenstahl.h"
#include "usb_msc.h"
#include "led.h"
#include "drv_fram.h"
#include "drv_eeprom.h"
#include "drv_flash.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/*------------- MAIN -------------*/
int main(void) {

	es_init();
	led_init();
	fram_init();
	eeprom_init();
	flash_init();

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);

	while (1) {
		tud_task(); // tinyusb device task
		led_task();
	}

}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  // a fresh enumeration re-inserts the medium on every LUN, so that
  // replugging the device recovers from a host-initiated eject
  usb_msc_reset_eject();
  led_set_link(LED_LINK_MOUNTED);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  led_set_link(LED_LINK_UNMOUNTED);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  led_set_link(LED_LINK_SUSPENDED);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  led_set_link(tud_mounted() ? LED_LINK_MOUNTED : LED_LINK_UNMOUNTED);
}
