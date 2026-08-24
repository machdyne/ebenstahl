#include <stdbool.h>

void eeprom_init(void);
void eeprom_read(int cs_gpio, char *buf, int addr, int len);
void eeprom_write(int cs_gpio, int addr, char *buf, int len);
