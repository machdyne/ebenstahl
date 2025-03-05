#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ebenstahl.h"

void es_init(void) {

	// rgb led
	gpio_init(ES_LEDR);
	gpio_set_dir(ES_LEDR, true);
	gpio_init(ES_LEDG);
	gpio_set_dir(ES_LEDG, true);
	gpio_init(ES_LEDB);
	gpio_set_dir(ES_LEDB, true);

	gpio_put(ES_LEDR, 1);
	gpio_put(ES_LEDG, 1);
	gpio_put(ES_LEDB, 1);

	// write protect switch
	gpio_init(ES_WP);
	gpio_set_dir(ES_WP, false);
	gpio_set_pulls(ES_WP, true, false);

	// spi bus
	gpio_init(ES_CS0);
	gpio_init(ES_CS1);
	gpio_init(ES_CS2);
	gpio_init(ES_CS3);
	gpio_init(ES_CS4);
	gpio_init(ES_CS5);
	gpio_init(ES_CS6);
	gpio_init(ES_CS7);
	gpio_init(ES_CS8);
	gpio_init(ES_CS9);
	gpio_init(ES_CS10);
	gpio_init(ES_CS11);
	gpio_init(ES_CS12);
	gpio_init(ES_CS13);
	gpio_init(ES_CS14);
	gpio_init(ES_CS15);

	gpio_init(ES_MISO);
	gpio_init(ES_MOSI);
	gpio_init(ES_SCK);

	gpio_set_function(ES_MISO, GPIO_FUNC_SPI);
	gpio_set_function(ES_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(ES_SCK, GPIO_FUNC_SPI);

	gpio_set_dir(ES_MOSI, 1);
	gpio_set_dir(ES_SCK, 1);

	gpio_set_dir(ES_CS0, 1);
	gpio_set_dir(ES_CS1, 1);
	gpio_set_dir(ES_CS2, 1);
	gpio_set_dir(ES_CS3, 1);
	gpio_set_dir(ES_CS4, 1);
	gpio_set_dir(ES_CS5, 1);
	gpio_set_dir(ES_CS6, 1);
	gpio_set_dir(ES_CS7, 1);
	gpio_set_dir(ES_CS8, 1);
	gpio_set_dir(ES_CS9, 1);
	gpio_set_dir(ES_CS10, 1);
	gpio_set_dir(ES_CS11, 1);
	gpio_set_dir(ES_CS12, 1);
	gpio_set_dir(ES_CS13, 1);
	gpio_set_dir(ES_CS14, 1);
	gpio_set_dir(ES_CS15, 1);

	gpio_put(ES_CS0, 1);
	gpio_put(ES_CS1, 1);
	gpio_put(ES_CS2, 1);
	gpio_put(ES_CS3, 1);
	gpio_put(ES_CS4, 1);
	gpio_put(ES_CS5, 1);
	gpio_put(ES_CS6, 1);
	gpio_put(ES_CS7, 1);
	gpio_put(ES_CS8, 1);
	gpio_put(ES_CS9, 1);
	gpio_put(ES_CS10, 1);
	gpio_put(ES_CS11, 1);
	gpio_put(ES_CS12, 1);
	gpio_put(ES_CS13, 1);
	gpio_put(ES_CS14, 1);
	gpio_put(ES_CS15, 1);

	spi_init(ES_SPI, 10000 * 1000);	// 10 MHz

}

bool es_wp_is_on(void) {

	bool wp = gpio_get(ES_WP);

	return wp;

}
