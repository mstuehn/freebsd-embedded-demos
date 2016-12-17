#ifndef __TMP102_H__
#define __TMP102_H__

#define	TMP102_DEFAULT_ADDR	0x48
#define	TMP102_INVALID_HANDLE	NULL

#define	TMP102_REG_TEMP		0
#define	TMP102_REG_CONF		1
#define		TMP102_CONF_EM		(1 << 4)
#define	TMP102_REG_TEMP_LOW	2
#define	TMP102_REG_TEMP_HIGH	3

struct tmp102_handle {
	int fd;
	uint8_t addr;
};

typedef struct tmp102_handle* tmp102_handle_t;

tmp102_handle_t tmp102_open(const char *i2cdev, int addr);
void tmp102_close(tmp102_handle_t);
int tmp102_read_temp(tmp102_handle_t h, int *temp);
int tmp102_read_temp_bracket(tmp102_handle_t h, int *lower, int *higher);

/* Low-level API */
int tmp102_read_register(tmp102_handle_t h, uint8_t reg, uint16_t *val);
int tmp102_write_register(tmp102_handle_t h, uint8_t reg, uint16_t val);

#endif /* __TMP102_H__ */
