/*-
 * Copyright (c) 2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
