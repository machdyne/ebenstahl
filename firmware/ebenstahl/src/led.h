#ifndef _LED_H_
#define _LED_H_

// USB link state
typedef enum {
	LED_LINK_UNMOUNTED = 0,
	LED_LINK_MOUNTED,
	LED_LINK_SUSPENDED,
} led_link_t;

// SCSI medium state
typedef enum {
	LED_MEDIUM_PRESENT = 0,
	LED_MEDIUM_EJECTED,
} led_medium_t;

void led_init(void);
void led_task(void);

void led_set_link(led_link_t link);
void led_set_medium(led_medium_t medium);

// momentary overlays
void led_note_activity(void);
void led_note_fault(void);

#endif	// _LED_H_
