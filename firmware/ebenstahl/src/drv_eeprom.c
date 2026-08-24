/* 
 * Ebenstahl EEPROM driver
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * Tested with:
 *   M95P32-IXMNT/E (32Mbit / 4MB SPI EEPROM)
 *
 * The M95P32 supports self-timed byte and page writes (up to 512 bytes,
 * the page size) that automatically erase-then-program, via the PGWR
 * opcode. eeprom_write() splits an arbitrary-length, arbitrarily-aligned
 * transfer into per-page chunks and waits for each page's internal write
 * cycle (WIP) to finish before issuing the next one, so callers can pass
 * any length/offset without worrying about page boundaries.
 *
 */

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "ebenstahl.h"
#include "drv_eeprom.h"

// M95P32 opcodes (24-bit / 3 address byte addressing)
#define EEPROM_CMD_WREN		0x06	// write enable
#define EEPROM_CMD_WRDI		0x04	// write disable
#define EEPROM_CMD_RDSR		0x05	// read status register
#define EEPROM_CMD_WRSR		0x01	// write status register
#define EEPROM_CMD_READ		0x03	// read data, single output
#define EEPROM_CMD_PGWR		0x02	// page write (self-timed auto erase + program)
#define EEPROM_CMD_PGPR		0x0A	// page program (no auto erase)
#define EEPROM_CMD_PGER		0xDB	// page erase (512 bytes)
#define EEPROM_CMD_CHER		0xC7	// chip erase

// M95P32 status register bit layout:
//   b7      b6   b5   b4   b3   b2   b1   b0
//   SRWD    TB   BP2  BP1  BP0  WEL  WIP
#define EEPROM_SR_WIP		0x01	// write in progress (read-only)
#define EEPROM_SR_WEL		0x02	// write enable latch (read-only)
#define EEPROM_SR_BP0		0x04	// block protection bit 0
#define EEPROM_SR_BP1		0x08	// block protection bit 1
#define EEPROM_SR_BP2		0x10	// block protection bit 2
#define EEPROM_SR_TB		0x20	// top/bottom protection select
#define EEPROM_SR_SRWD		0x80	// status register write disable

#define EEPROM_PAGE_SIZE	512

void eeprom_init(void) {
	// nothing to do here; SPI bus is already initialized by es_init()
}

static uint8_t eeprom_read_sr(int cs_gpio) {

	uint8_t cmdbuf[1] = { EEPROM_CMD_RDSR };
	uint8_t sr = 0;

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 1);
	spi_read_blocking(ES_SPI, 0x00, &sr, 1);
	gpio_put(cs_gpio, 1);

	return sr;

}

// block until the internal (self-timed) write cycle has completed
static void eeprom_wait_wip(int cs_gpio) {

	while (eeprom_read_sr(cs_gpio) & EEPROM_SR_WIP) {
		tight_loop_contents();
	}

}

static void eeprom_write_enable(int cs_gpio) {

	uint8_t cmdbuf[1] = { EEPROM_CMD_WREN };

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 1);
	gpio_put(cs_gpio, 1);

}

void eeprom_read(int cs_gpio, char *buf, int addr, int len) {

	uint8_t cmdbuf[4] = { EEPROM_CMD_READ, addr >> 16, addr >> 8, addr & 0xff };

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 4);
	spi_read_blocking(ES_SPI, 0x00, buf, len);
	gpio_put(cs_gpio, 1);

}

// write up to EEPROM_PAGE_SIZE bytes; addr..addr+len-1 must fall within a
// single page (the caller, eeprom_write() below, guarantees this)
static void eeprom_page_write(int cs_gpio, int addr, char *buf, int len) {

	uint8_t cmdbuf[4] = { EEPROM_CMD_PGWR, addr >> 16, addr >> 8, addr & 0xff };

	eeprom_write_enable(cs_gpio); // auto-disabled after each write

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 4);
	spi_write_blocking(ES_SPI, buf, len);
	gpio_put(cs_gpio, 1);

	// PGWR is self-timed (auto erase + program); wait for it to finish
	// before returning, so callers can safely chain writes and reads.
	eeprom_wait_wip(cs_gpio);

}

// write an arbitrary-length, arbitrarily-aligned buffer, transparently
// splitting the transfer at 512-byte page boundaries as needed. each
// resulting page write uses the self-timed auto erase + program opcode,
// so no separate erase step is required for byte or page writes.
void eeprom_write(int cs_gpio, int addr, char *buf, int len) {

	while (len > 0) {

		int page_offset    = addr % EEPROM_PAGE_SIZE;
		int page_remaining = EEPROM_PAGE_SIZE - page_offset;
		int chunk = (len < page_remaining) ? len : page_remaining;

		eeprom_page_write(cs_gpio, addr, buf, chunk);

		addr += chunk;
		buf  += chunk;
		len  -= chunk;

	}

}
