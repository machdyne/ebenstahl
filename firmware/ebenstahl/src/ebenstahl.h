#ifndef _EBENSTAHL_H_
#define _EBENSTAHL_H_

#include <stdbool.h>

void es_init(void);
bool es_wp_is_on(void);

//#define EBENSTAHL

#ifdef EBENSTAHL

// ebenstahl

#define ES_SPI		spi0

#define ES_LEDR	6
#define ES_LEDG	4
#define ES_LEDB	2

#define ES_WP		9

#define ES_CS7		10
#define ES_CS6		11
#define ES_CS5		12
#define ES_CS4		13
#define ES_CS3		14
#define ES_CS2		15
#define ES_CS1		16
#define ES_CS0		17

#define ES_SCK		18
#define ES_MOSI	19
#define ES_MISO	20

#define ES_CS15	22
#define ES_CS14	23
#define ES_CS13	24
#define ES_CS12	25
#define ES_CS11	26
#define ES_CS10	27
#define ES_CS9		28
#define ES_CS8		29

#define ES_DRV_NONE		0
#define ES_DRV_FRAM		1
#define ES_DRV_FLASH		2
#define ES_DRV_EEPROM	3

#else

// blaustahl, kaltstahl, rosastahl, etc.

#define ES_LED		9

#define ES_SPI		spi1
#define ES_MOSI	11
#define ES_MISO	12
#define ES_CS0		13
#define ES_SCK		14

#endif

#endif
