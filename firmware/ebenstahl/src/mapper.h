int mapper_luns(void);
int mapper_lun_size(uint8_t lun);
int mapper_rec(uint8_t lun, uint32_t addr);
int mapper_get_drv(uint8_t lun, uint32_t addr);
int mapper_get_cs(uint8_t lun, uint32_t addr);
