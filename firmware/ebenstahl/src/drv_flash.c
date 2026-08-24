/* 
 * Ebenstahl NOR Flash driver
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * Tested with:
 *   SST26VF016B-80E/SN (Microchip SuperFlash, 16Mbit/2MB SPI NOR)
 *
 * Should also work with other SPI NOR flash chips that use 3-byte
 * (24-bit) addressing and the standard READ/WREN/RDSR/WRSR/PP/SE
 * opcodes below, as long as their page size is <= FLASH_PAGE_SIZE and
 * their erase sector size is a multiple of FLASH_SECTOR_SIZE.
 *
 * NOR flash write economics are fundamentally different from the FRAM
 * and EEPROM drivers: a program operation can only clear bits
 * (1 -> 0); setting them back to 1 requires erasing an entire sector
 * first (4KB here), and erase always clears the whole sector, not
 * just the bytes being written. But USB-MSC WRITE10 commands arrive
 * as arbitrary, unaligned byte ranges (512-byte blocks scattered
 * across FAT tables, directory entries, and file data).
 *
 * To keep that mismatch entirely out of usb_msc.c and mapper.c,
 * flash_write() below implements the full read-modify-erase-write
 * cycle itself, one 4KB sector at a time:
 *
 *   1. read the whole sector containing the target bytes
 *   2. splice the new bytes into a local copy
 *   3. if nothing actually changed, stop (saves an erase/program
 *      cycle and some flash wear -- useful since filesystems tend to
 *      rewrite the same FAT/directory sectors repeatedly)
 *   4. otherwise erase the sector and reprogram it in page-sized
 *      (256B) chunks
 *
 * so callers can write any length at any offset, exactly like
 * fram_write()/eeprom_write(), without worrying about erase or page
 * granularity.
 *
 * Many SPI NOR chips (including the SST26VF family) also power up
 * with the entire array write-protected and require an unlock
 * sequence before the first erase/program will succeed. flash_write()
 * performs this lazily and once per chip select (tracked with a
 * bitmask), so no separate init pass over chip selects is needed.
 *
 */

#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "ebenstahl.h"
#include "drv_flash.h"

// generic SPI NOR opcodes (24-bit / 3 address byte addressing)
#define FLASH_CMD_WREN		0x06	// write enable
#define FLASH_CMD_WRDI		0x04	// write disable
#define FLASH_CMD_RDSR		0x05	// read status register
#define FLASH_CMD_WRSR		0x01	// write status register
#define FLASH_CMD_READ		0x03	// read data, single output
#define FLASH_CMD_PP		0x02	// page program
#define FLASH_CMD_SE		0x20	// sector erase (4KB)
#define FLASH_CMD_ULBPR		0x98	// global block-protection unlock
					// (SST26 "SuperFlash" and many other
					// vendors' "individual block
					// protection" parts; harmlessly
					// ignored by chips without it)

#define FLASH_SR_BUSY		0x01	// write/erase in progress (read-only)
#define FLASH_SR_WEL		0x02	// write enable latch (read-only)

#define FLASH_PAGE_SIZE		256
#define FLASH_SECTOR_SIZE	4096

// tracks which chip selects have already been unlocked this session,
// one bit per GPIO number (RP2040 has 30 GPIOs, well within 32 bits)
static uint32_t flash_unlocked_mask = 0;

// scratch buffer for the read-modify-erase-write cycle; static so it
// doesn't blow the (potentially small) stack of the calling context
static uint8_t flash_sector_buf[FLASH_SECTOR_SIZE];

void flash_init(void) {
	// nothing to do here; SPI bus is already initialized by es_init().
	// per-chip unlock happens lazily, see flash_unlock_once() below.
}

static uint8_t flash_read_sr(int cs_gpio) {

	uint8_t cmdbuf[1] = { FLASH_CMD_RDSR };
	uint8_t sr = 0;

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 1);
	spi_read_blocking(ES_SPI, 0x00, &sr, 1);
	gpio_put(cs_gpio, 1);

	return sr;

}

// block until the current erase/program cycle has completed
static void flash_wait_busy(int cs_gpio) {

	while (flash_read_sr(cs_gpio) & FLASH_SR_BUSY) {
		tight_loop_contents();
	}

}

static void flash_write_enable(int cs_gpio) {

	uint8_t cmdbuf[1] = { FLASH_CMD_WREN };

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 1);
	gpio_put(cs_gpio, 1);

}

// clear write protection for a chip, once per chip select per session.
// two mechanisms are covered since different chips use different
// schemes: classic status-register BP bits (cleared via WRSR), and
// SST26-style individual block protection (cleared via ULBPR). a chip
// that doesn't implement one of these simply ignores that opcode.
static void flash_unlock_once(int cs_gpio) {

	if (cs_gpio < 0 || cs_gpio >= 32) return;
	if (flash_unlocked_mask & (1u << cs_gpio)) return;

	// clear classic block-protection bits in the status register
	flash_write_enable(cs_gpio);
	uint8_t wrsr[2] = { FLASH_CMD_WRSR, 0x00 };
	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, wrsr, 2);
	gpio_put(cs_gpio, 1);
	flash_wait_busy(cs_gpio);

	// clear SST26-style individual block protection
	flash_write_enable(cs_gpio);
	uint8_t ulbpr[1] = { FLASH_CMD_ULBPR };
	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, ulbpr, 1);
	gpio_put(cs_gpio, 1);
	flash_wait_busy(cs_gpio);

	flash_unlocked_mask |= (1u << cs_gpio);

}

void flash_read(int cs_gpio, char *buf, int addr, int len) {

	uint8_t cmdbuf[4] = { FLASH_CMD_READ, addr >> 16, addr >> 8, addr & 0xff };

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 4);
	spi_read_blocking(ES_SPI, 0x00, buf, len);
	gpio_put(cs_gpio, 1);

}

static void flash_sector_erase(int cs_gpio, int addr) {

	uint8_t cmdbuf[4] = { FLASH_CMD_SE, addr >> 16, addr >> 8, addr & 0xff };

	flash_write_enable(cs_gpio);

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 4);
	gpio_put(cs_gpio, 1);

	flash_wait_busy(cs_gpio);

}

// program up to FLASH_PAGE_SIZE bytes; addr must be page-aligned (all
// callers below only ever program whole, aligned pages)
static void flash_page_program(int cs_gpio, int addr, uint8_t *buf, int len) {

	uint8_t cmdbuf[4] = { FLASH_CMD_PP, addr >> 16, addr >> 8, addr & 0xff };

	flash_write_enable(cs_gpio);

	gpio_put(cs_gpio, 0);
	spi_write_blocking(ES_SPI, cmdbuf, 4);
	spi_write_blocking(ES_SPI, buf, len);
	gpio_put(cs_gpio, 1);

	flash_wait_busy(cs_gpio);

}

// erase and reprogram one full sector, splicing in [buf, buf+len) at
// sector_offset. len must not extend past the end of the sector.
static void flash_rmw_sector(int cs_gpio, int sector_addr, int sector_offset,
    uint8_t *buf, int len) {

	flash_read(cs_gpio, (char *) flash_sector_buf, sector_addr, FLASH_SECTOR_SIZE);

	// nothing to do if the sector already holds this data
	if (memcmp(flash_sector_buf + sector_offset, buf, len) == 0) return;

	memcpy(flash_sector_buf + sector_offset, buf, len);

	flash_unlock_once(cs_gpio);
	flash_sector_erase(cs_gpio, sector_addr);

	for (int p = 0; p < FLASH_SECTOR_SIZE; p += FLASH_PAGE_SIZE) {
		flash_page_program(cs_gpio, sector_addr + p,
		    flash_sector_buf + p, FLASH_PAGE_SIZE);
	}

}

// write an arbitrary-length, arbitrarily-aligned buffer, transparently
// handling the read-modify-erase-write cycle at 4KB sector boundaries.
void flash_write(int cs_gpio, int addr, char *buf, int len) {

	uint8_t *buf_ptr = (uint8_t *) buf;

	while (len > 0) {

		int sector_addr   = addr - (addr % FLASH_SECTOR_SIZE);
		int sector_offset = addr - sector_addr;
		int sector_space  = FLASH_SECTOR_SIZE - sector_offset;
		int chunk = (len < sector_space) ? len : sector_space;

		flash_rmw_sector(cs_gpio, sector_addr, sector_offset, buf_ptr, chunk);

		addr    += chunk;
		buf_ptr += chunk;
		len     -= chunk;

	}

}
