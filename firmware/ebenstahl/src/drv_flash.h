#include <stdbool.h>

void flash_init(void);
void flash_read(int cs_gpio, char *buf, int addr, int len);
void flash_write(int cs_gpio, int addr, char *buf, int len);
