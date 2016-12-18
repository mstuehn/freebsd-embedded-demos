#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <dev/iicbus/iic.h>
#include "tmp102.h"

static int
reg_to_temp(uint16_t val, int extended)
{
	int sign, temp;
	int bits;

	if (extended) {
		bits = 13;
		val >>= 3;
	}
	else
		val >>= 4;

	if (val & (1 << (bits - 1))) {
		sign = -1;
		val = (1 << bits) - val;
	}
	else
		sign = 1;

	temp = val * 1000 / 16;

	return (temp*sign);
}

tmp102_handle_t
tmp102_open(const char *i2cdev, int addr)
{
	tmp102_handle_t h;
	int fd;

	fd = open(i2cdev, O_RDWR);
	if (fd < 0)
		return (TMP102_INVALID_HANDLE);

	h = (tmp102_handle_t)malloc(sizeof(*h));
	if (h == NULL)
		return (TMP102_INVALID_HANDLE);

	h->addr = (addr << 1);
	h->fd = fd;

	return (h);
}

void
tmp102_close(tmp102_handle_t h)
{
	close(h->fd);
	free(h);
}

int
tmp102_read_register(tmp102_handle_t h, uint8_t reg, uint16_t *val)
{
	uint8_t bytes[2];
	int error;
	struct iic_rdwr_data data;
	struct iic_msg msgs[2] = {
		{0, IIC_M_WR, 1, &reg},
		{0, IIC_M_RD, 2, bytes},
	};

	if (h == TMP102_INVALID_HANDLE)
		return (-1);
	if (val == NULL)
		return (-1);

	msgs[0].slave = h->addr;
	msgs[1].slave = h->addr;

	data.nmsgs = 2;
	data.msgs = msgs;
	error = ioctl(h->fd, I2CRDWR, &data);
	if (error)
		return (-1);

	*val = ((bytes[0] << 8) | bytes[1]);
	return (0);
}

int
tmp102_write_register(tmp102_handle_t h, uint8_t reg, uint16_t val)
{
	uint8_t bytes[2];
	int error;
	struct iic_rdwr_data data;
	struct iic_msg msgs[2] = {
		{0, IIC_M_WR, 1, &reg},
		{0, IIC_M_RD, 2, bytes},
	};

	if (h == TMP102_INVALID_HANDLE)
		return (-1);

	bytes[0] = (val >> 8) & 0xff;
	bytes[1] = val & 0xff;

	msgs[0].slave = h->addr;
	msgs[1].slave = h->addr;

	data.nmsgs = 2;
	data.msgs = msgs;
	error = ioctl(h->fd, I2CRDWR, &data);
	if (error)
		return (-1);

	return (0);
}

int
tmp102_read_temp(tmp102_handle_t h, int *temp)
{
	uint16_t conf_reg, temp_reg;
	int extended;
	int err;

	if (h == TMP102_INVALID_HANDLE)
		return (-1);

	if ((err = tmp102_read_register(h, TMP102_REG_CONF, &conf_reg)))
		return (err);
	extended = (conf_reg & TMP102_CONF_EM) ? 1 : 0;
	if ((err = tmp102_read_register(h, TMP102_REG_TEMP, &temp_reg)))
		return (err);

	*temp = reg_to_temp(temp_reg, extended);

	return (0);
}

int
tmp102_read_temp_bracket(tmp102_handle_t h, int *lower, int *higher)
{
	uint16_t conf_reg, low_reg, high_reg;
	int extended;
	int err;

	if (h == TMP102_INVALID_HANDLE)
		return (-1);

	if ((err = tmp102_read_register(h, TMP102_REG_CONF, &conf_reg)))
		return (err);
	extended = (conf_reg & TMP102_CONF_EM) ? 1 : 0;
	if ((err = tmp102_read_register(h, TMP102_REG_TEMP_LOW, &low_reg)))
		return (err);
	if ((err = tmp102_read_register(h, TMP102_REG_TEMP_HIGH, &high_reg)))
		return (err);

	*lower = reg_to_temp(low_reg, extended);
	*higher = reg_to_temp(high_reg, extended);

	return (0);
}
