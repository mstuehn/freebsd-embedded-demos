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

#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <dev/iicbus/iic.h>

#include "font.h"
#include "ssd1306.h"

#define SSD1306_CMD		0x00
#define SSD1306_DATA		0x40
#define	SSD1306_SETCONTRAST	0x81
#define	SSD1306_DISPLAYALLON_RESUME	0xA4
#define	SSD1306_DISPLAYALLON	0xA5
#define	SSD1306_NORMALDISPLAY	0xA6
#define	SSD1306_INVERTDISPLAY	0xA7
#define	SSD1306_DISPLAYOFF	0xAE
#define	SSD1306_DISPLAYON	0xAF
#define	SSD1306_SETDISPLAYOFFSET	0xD3
#define	SSD1306_SETCOMPINS	0xDA
#define	SSD1306_SETVCOMDETECT	0xDB
#define	SSD1306_SETDISPLAYCLOCKDIV	0xD5
#define	SSD1306_SETPRECHARGE	0xD9
#define	SSD1306_SETMULTIPLEX	0xA8
#define	SSD1306_SETLOWCOLUMN	0x00
#define	SSD1306_SETHIGHCOLUMN	0x10
#define	SSD1306_SETSTARTLINE	0x40
#define	SSD1306_MEMORYMODE	0x20
#define	SSD1306_COLUMNADDR	0x21
#define	SSD1306_PAGEADDR	0x22
#define	SSD1306_COMSCANINC	0xC0
#define	SSD1306_COMSCANDEC	0xC8
#define	SSD1306_SEGREMAP	0xA0
#define	SSD1306_CHARGEPUMP	0x8D

#define FONT_WIDTH	8

struct ssd1306_handle {
	ssd1306_model	model;
	int		flags;
	int		iic_fd;
	uint8_t		iic_slave_adr;
	int		width;
	int		height;
	/* Virtual screen, 1 byte per pixel */
	uint8_t		*screen;
	int		screen_size;
	/* View port data in SSD1306-compatible format */
	uint8_t		*scratch;
	int		scratch_size;
	ssd1306_font	font;
	ssd1306_vccstate vccstate;
};

static int
ssd1306_command(ssd1306_handle_t h, uint8_t cmd)
{
	uint8_t buf[] = { SSD1306_CMD, cmd };
	struct iic_msg msg[1];
	struct iic_rdwr_data rdwr;

	msg[0].slave = h->iic_slave_adr << 1;
	msg[0].flags = IIC_M_WR;
	msg[0].len = sizeof(buf);
	msg[0].buf = buf;

	rdwr.nmsgs = 1;
	rdwr.msgs = msg;

	if (ioctl(h->iic_fd, I2CRDWR, &rdwr) < 0) {
		perror("ioctl(I2CRDWR) failed");
		return (-1);
	}
	else return 0;
}

static int
ssd1306_data(ssd1306_handle_t h, uint8_t *data, int len)
{
	struct iic_msg msg[2];
	struct iic_rdwr_data rdwr;

	uint8_t buf[len+1];
	buf[0] = SSD1306_DATA;
	memcpy(&buf[1], data, len);
	msg[0].slave = h->iic_slave_adr << 1;
	msg[0].flags = IIC_M_WR;
	msg[0].len = len+1;
	msg[0].buf = buf;

	rdwr.nmsgs = 1;
	rdwr.msgs = msg;

	if (ioctl(h->iic_fd, I2CRDWR, &rdwr) < 0) {
		perror("ioctl(I2CRDWR) failed");
		return (-1);
	}
	else return 0;
}

static int
ssd1306_reset(ssd1306_handle_t h)
{
	ssd1306_command(h, 0x2E); /* Stop scrolling */

	return (0);
}

static void
ssd1306_initialize_128x64(ssd1306_handle_t h)
{
	ssd1306_reset(h);

	ssd1306_command(h, SSD1306_DISPLAYOFF);
	ssd1306_command(h, SSD1306_SETDISPLAYCLOCKDIV);
	ssd1306_command(h, 0x80); /* the suggested ratio 0x80 */
	ssd1306_command(h, SSD1306_SETMULTIPLEX);
	ssd1306_command(h, 0x3F);
	ssd1306_command(h, SSD1306_SETDISPLAYOFFSET);
	ssd1306_command(h, 0x0); /* no offset */
	ssd1306_command(h, SSD1306_SETSTARTLINE | 0x0); /* line #0 */
	ssd1306_command(h, SSD1306_CHARGEPUMP);
	if (h->vccstate == SSD1306_EXTERNALVCC)
		ssd1306_command(h, 0x10);
	else
		ssd1306_command(h, 0x14);
	ssd1306_command(h, SSD1306_MEMORYMODE);
	ssd1306_command(h, 0x00); /* 0x0 act like ks0108 */
	ssd1306_command(h, SSD1306_SEGREMAP | 0x1);
	ssd1306_command(h, SSD1306_COMSCANDEC);
	ssd1306_command(h, SSD1306_SETCOMPINS);
	ssd1306_command(h, 0x12);
	ssd1306_command(h, SSD1306_SETCONTRAST);
	ssd1306_command(h, 0x8F);
	ssd1306_command(h, SSD1306_SETPRECHARGE);
	if (h->vccstate == SSD1306_EXTERNALVCC)
		ssd1306_command(h, 0x22);
	else
		ssd1306_command(h, 0xF1);
	ssd1306_command(h, SSD1306_SETVCOMDETECT);
	ssd1306_command(h, 0x40);
	ssd1306_command(h, SSD1306_DISPLAYALLON_RESUME);
	if (h->flags & SSD1306_FLAG_INVERSE)
		ssd1306_command(h, SSD1306_INVERTDISPLAY);
	else
		ssd1306_command(h, SSD1306_NORMALDISPLAY);
}

void
ssd1306_clear(ssd1306_handle_t h)
{

	memset(h->screen, 0, h->screen_size);
}

int
ssd1306_refresh(ssd1306_handle_t h)
{
	int x, y;
	int sx, sy;
	uint8_t *page;
	int bit, onoff;

	memset(h->scratch, 0, h->scratch_size);
	for (x = 0; x < h->width; x++) {
		for (y = 0; y < h->height; y++) {
			if (h->flags & SSD1306_FLAG_ROTATE) {
				sx = h->width - x - 1;
				sy = h->height - y - 1;
			} else {
				sx = x;
				sy = y;
			}
			page = h->scratch + ((sy / 8) * h->width + sx);
			bit = sy % 8;
			onoff = h->screen[y * h->width + x];
			if (onoff)
				*page |= (1 << bit);
			else
				*page &= ~(1 << bit);
		}
	}

	ssd1306_command(h, SSD1306_COLUMNADDR);
	ssd1306_command(h, 0);
	ssd1306_command(h, h->width - 1);
	ssd1306_command(h, SSD1306_PAGEADDR);
	ssd1306_command(h, 0);
	ssd1306_command(h, h->height/8 - 1);
	ssd1306_data(h, h->scratch, h->scratch_size);

	return (0);
}

void
ssd1306_scroll(ssd1306_handle_t h)
{
	ssd1306_command(h, 0x2E); /* Stop */

	ssd1306_command(h, 0x29); /* configure */
	ssd1306_command(h, 0x00); /* Dummy */
	ssd1306_command(h, 0x00); /* from Page0 */
	ssd1306_command(h, 0x00); /* 5frames */
	ssd1306_command(h, 0x00); /* to Page7 */
	ssd1306_command(h, 0x01); /* 0 rows ?*/

	ssd1306_command(h, 0x2F); /* Start */
}

int
ssd1306_width(ssd1306_handle_t h)
{
	return h->width;
}

int ssd1306_height(ssd1306_handle_t h)
{
	return h->height;
}

int ssd1306_font_width(ssd1306_handle_t h)
{
	return FONT_WIDTH;
}

int ssd1306_font_height(ssd1306_handle_t h)
{
	switch (h->font) {
	case SSD1306_FONT_8:
		return (8);
		break;
	case SSD1306_FONT_14:
		return (14);
		break;
	case SSD1306_FONT_16:
		return (16);
		break;
	default:
		return (-1);
	}
}

int
ssd1306_on(ssd1306_handle_t h)
{

	return ssd1306_command(h, SSD1306_DISPLAYON);
}

int
ssd1306_off(ssd1306_handle_t h)
{
	return ssd1306_command(h, SSD1306_DISPLAYOFF);
}

void
ssd1306_putpixel(ssd1306_handle_t h, int x, int y, int v)
{
	if ((x < 0) || (y < 0))
		return;
	if ((x >= h->width) || (y >= h->height))
		return;

	h->screen[y * h->width + x] = v;
}

void
ssd1306_putchar(ssd1306_handle_t h, int x, int y, unsigned char c)
{
	int cx, cy;
	uint8_t row;
	uint8_t *font;
	int font_height;

	switch (h->font) {
	case SSD1306_FONT_8:
		font = dflt_font_8;
		font_height = 8;
		break;
	case SSD1306_FONT_14:
		font = dflt_font_14;
		font_height = 14;
		break;
	case SSD1306_FONT_16:
		font = dflt_font_16;
		font_height = 16;
		break;
	default:
		return;
	}

	for (cy = 0; cy < font_height; cy++) {
		row = font[c * font_height + cy];
		for (cx = 0; cx < 8; cx++)
			ssd1306_putpixel(h, x + cx, y + cy, row & (1 << (7 - cx)));
	}
}

void
ssd1306_putstr(ssd1306_handle_t h, int x, int y, const char *s)
{
	int i;

	for (i = 0; i < strlen(s); i++)
		ssd1306_putchar(h, x + FONT_WIDTH * i, y, s[i]);
}

ssd1306_handle_t
ssd1306_open(const char *iicdev, ssd1306_model model, uint8_t slave_address, int flags)
{
	ssd1306_handle_t h;
	h = malloc(sizeof *h);
	if (h == NULL) {
		printf("Malloc failed\n");
		return (SSD1306_INVALID_HANDLE);
	}

	switch (model) {
	case SSD1306_MODEL_128X32:  /* not supported yet */
	case SSD1306_MODEL_96X16: /* not supported yet */
        break;
	case SSD1306_MODEL_128X64:
		h->width = 128;
		h->height = 64;
		break;
	default:
		printf("Device not supported\n");
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

	h->model = model;
	h->vccstate = SSD1306_SWITCHCAPVCC;
	h->font = SSD1306_FONT_16;
	h->flags = flags;

	h->iic_fd = open(iicdev, O_RDWR);
	if (h->iic_fd < 0) {
		free(h);
		perror("Could not open device\n");
		return (SSD1306_INVALID_HANDLE);
	}

	h->iic_slave_adr = slave_address;

	h->screen_size = h->width * h->height;
	h->scratch_size = h->width * h->height / 8;
	h->screen = malloc(h->screen_size);
	h->scratch = malloc(h->scratch_size);

	return (h);
}

void
ssd1306_close(ssd1306_handle_t h)
{
	free(h->screen);
	free(h->scratch);
	close(h->iic_fd);
	free(h);
}

int
ssd1306_initialize(ssd1306_handle_t h)
{
	switch (h->model) {
	case SSD1306_MODEL_128X32:  /* not supported yet */
	case SSD1306_MODEL_96X16: /* not supported yet */
		break;
	case SSD1306_MODEL_128X64:
		ssd1306_initialize_128x64(h);
		break;
	default:
		printf("LCD model not supported\n");
		return (-1);
	}
	return (0);
}

