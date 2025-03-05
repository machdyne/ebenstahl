/* 
 * Ebenstahl FRAM driver
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * Tested with:
 *   MB85RS2MTAPNF-G-BDERE1
 *
 */

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "ebenstahl.h"
#include "drv_fram.h"

#define FRAM_BIG	// 24-bit addresses

void fram_init(void) {
	// nothing to do here
}

void fram_read(int cs_gpio, char *buf, int addr, int len) {

	int i;
#ifdef FRAM_BIG
	uint8_t cmdbuf[4] = { 0x03, addr >> 16, addr >> 8, addr & 0xff };	// READ
#else
	uint8_t cmdbuf[3] = { 0x03, addr >> 8, addr & 0xff };	// READ
#endif

	gpio_put(cs_gpio, 0);
#ifdef FRAM_BIG
	spi_write_blocking(ES_SPI, cmdbuf, 4);
#else
	spi_write_blocking(ES_SPI, cmdbuf, 3);
#endif
	spi_read_blocking(ES_SPI, 0x00, buf, len);
	gpio_put(cs_gpio, 1);

}

void fram_write_enable(int cs_gpio) {

	uint8_t cmdbuf[1] = { 0x06 };

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 1); // WREN
	gpio_put(cs_gpio, 1);

}

void fram_write(int cs_gpio, int addr, char *buf, int len) {

#ifdef FRAM_BIG
	uint8_t cmdbuf[4] = { 0x02, addr >> 16, addr >> 8, addr & 0xff };	// WRITE
#else
	uint8_t cmdbuf[3] = { 0x02, addr >> 8, addr & 0xff };	// WRITE
#endif

	fram_write_enable(cs_gpio); // auto-disabled after each write

	gpio_put(cs_gpio, 0);
#ifdef FRAM_BIG
	spi_write_blocking(ES_SPI, cmdbuf, 4);
#else
	spi_write_blocking(ES_SPI, cmdbuf, 3);
#endif
	spi_write_blocking(ES_SPI, buf, len);
	gpio_put(cs_gpio, 1);

}
