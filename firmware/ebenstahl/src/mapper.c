/* 
 * Ebenstahl Mapper
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 */

#include "ebenstahl.h"

const es_map[16] = {

	// CS			LUN	DRIVER			SIZE
	{ ES_CS0,	0,		ES_DRV_FRAM,	262144 },
	{ ES_CS1,	0,		ES_DRV_FRAM,	262144 },
	{ ES_CS2,	0,		ES_DRV_FRAM,	262144 },
	{ ES_CS3,	0,		ES_DRV_FRAM,	262144 },

	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },

	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },

	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },

};

// given a LUN and address, return the correct driver
uint8_t mapper_get_drv(uint8_t lun, uint32_t addr) {
	return drv;
}

// given a LUN and address, return the correct chip select
uint8_t mapper_get_cs(uint8_t lun, uint32_t addr) {
	return cs;
}
