#include <stdbool.h>

void fram_init(void);
void fram_read(int cs_gpio, char *buf, int addr, int len);
void fram_write_enable(int cs_gpio);
void fram_write(int cs_gpio, int addr, char *buf, int len);
