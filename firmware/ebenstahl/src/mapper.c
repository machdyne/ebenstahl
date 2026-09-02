/* 
 * Ebenstahl Mapper
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 */

#include <stdint.h>

#include "ebenstahl.h"

const int es_map[16][4] = {

#ifdef EBENSTAHL

	// CS			LUN	DRIVER			SIZE
	{ ES_CS0,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS1,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS2,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS3,	0,		ES_DRV_FRAM,	524288 },

	{ ES_CS4,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS5,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS6,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS7,	0,		ES_DRV_FRAM,	524288 },

	{ ES_CS8,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS9,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS10,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS11,	0,		ES_DRV_FRAM,	524288 },

	{ ES_CS12,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS13,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS14,	0,		ES_DRV_FRAM,	524288 },
	{ ES_CS15,	0,		ES_DRV_FRAM,	524288 },

#else

	{ ES_CS0,	0,		ES_DRV_FRAM,	524288 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },

	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },

	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },

	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },
	{ 0, -1, 0, 0 },

#endif

};

int mapper_luns(void) {

	int max_lun = 0;

	for (int i = 0; i < 16; i++) {
		if (es_map[i][1] > max_lun) max_lun = es_map[i][1];
	}

	return(max_lun + 1);  

}  

int mapper_lun_size(uint8_t lun) {

	int size = 0;

	for (int i = 0; i < 16; i++) {
		if (es_map[i][1] == lun) size += es_map[i][3];
	}

	return(size);

}

// Returns the map entry index that owns the given LUN-absolute address,
// or -1 if not found.
int mapper_rec(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 16; i++) {
		if (es_map[i][1] == lun) {
			uint32_t entry_end = p + (uint32_t)es_map[i][3];
			if (addr >= p && addr < entry_end)
				return i;
			p = entry_end;
		}
	}

	return -1;

}

// Returns how many bytes remain in the current chip from addr onwards.
// Used by callers to split transfers at chip boundaries.
int mapper_remaining(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 16; i++) {
		if (es_map[i][1] == lun) {
			uint32_t entry_end = p + (uint32_t)es_map[i][3];
			if (addr >= p && addr < entry_end)
				return (int)(entry_end - addr);
			p = entry_end;
		}
	}

	return 0;

}

// Translates a LUN-absolute address to a chip-local address.
uint32_t mapper_get_local_addr(uint8_t lun, uint32_t addr) {

	uint32_t p = 0;

	for (int i = 0; i < 16; i++) {
		if (es_map[i][1] == lun) {
			uint32_t entry_end = p + (uint32_t)es_map[i][3];
			if (addr >= p && addr < entry_end)
				return addr - p;
			p = entry_end;
		}
	}

	return 0;

}

// given a LUN and address, return the correct driver
int mapper_get_drv(uint8_t lun, uint32_t addr) {
	int rec = mapper_rec(lun, addr);
	if (rec < 0) return ES_DRV_NONE;
	return es_map[rec][2];
}

// given a LUN and address, return the correct chip select
int mapper_get_cs(uint8_t lun, uint32_t addr) {
	int rec = mapper_rec(lun, addr);
	if (rec < 0) return -1;
	return es_map[rec][0];
}
