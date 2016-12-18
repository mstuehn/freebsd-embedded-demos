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
#include <libgpio.h>
#include <string.h>
#include <sys/spigenio.h>

#include "font.h"
#include "ssd1306.h"

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
	int		spi_fd;
	/* Reset pin */
	gpio_handle_t	gpio_reset;
	int		gpio_reset_pin;
	/* Data/Command switch pin */
	gpio_handle_t	gpio_dc;
	int		gpio_dc_pin;
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

int
ssd1306_spi_transfer(ssd1306_handle_t h, uint8_t *data, int len)
{
	struct spigen_transfer transfer;
	uint8_t foo;

	/*
	 * Note: data will be overwritten, can't be const.
	 * If you need to keep the data intact - create copy
	 */
	transfer.st_command.iov_base = data;
	transfer.st_command.iov_len = len;
	transfer.st_data.iov_base = NULL;
	transfer.st_data.iov_len = 0;
	if (ioctl(h->spi_fd, SPIGENIOC_TRANSFER, &transfer) < 0)
		return (-1);

	return (0);
}

static int
ssd1306_command(ssd1306_handle_t h, uint8_t cmd)
{
	if (gpio_pin_low(h->gpio_dc, h->gpio_dc_pin))
		return (-1);
	if (ssd1306_spi_transfer(h, &cmd, 1))
		return (-1);

	return (0);
}

static int
ssd1306_data(ssd1306_handle_t h, uint8_t *data, int len)
{
	if (gpio_pin_high(h->gpio_dc, h->gpio_dc_pin))
		return (-1);
	if (ssd1306_spi_transfer(h, data, len))
		return (-1);

	return (0);
}

static int
ssd1306_reset(ssd1306_handle_t h)
{
	if (gpio_pin_high(h->gpio_reset, h->gpio_reset_pin))
		return (-1);
	usleep(999);
	if (gpio_pin_low(h->gpio_reset, h->gpio_reset_pin))
		return (-1);
	usleep(10000);
	if (gpio_pin_high(h->gpio_reset, h->gpio_reset_pin))
		return (-1);

	return (0);
}

static void
ssd1306_initialize_128x32(ssd1306_handle_t h)
{
	ssd1306_reset(h);

	ssd1306_command(h, SSD1306_DISPLAYOFF);
	ssd1306_command(h, SSD1306_SETDISPLAYCLOCKDIV);
	ssd1306_command(h, 0x80); /* the suggested ratio 0x80 */
	ssd1306_command(h, SSD1306_SETMULTIPLEX);
	ssd1306_command(h, 0x1F);
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
	ssd1306_command(h, 0x02);
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
ssd1306_open(const char *spiodev, ssd1306_model model, int gpio_reset_unit,
    int gpio_reset_pin, int gpio_dc_unit, int gpio_dc_pin, int flags)
{
	ssd1306_handle_t h;
	h = malloc(sizeof *h);
	if (h == NULL)
		return (SSD1306_INVALID_HANDLE);

	switch (model) {
	case SSD1306_MODEL_128X32:
		h->width = 128;
		h->height = 32;
		break;
	case SSD1306_MODEL_96X16: /* not supported yet */
	case SSD1306_MODEL_128X64: /* not supported yet */
	default:
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

	h->model = model;
	h->vccstate = SSD1306_SWITCHCAPVCC;
	h->font = SSD1306_FONT_16;
	h->flags = flags;

	h->spi_fd = open(spiodev, O_RDWR);
	if (h->spi_fd < 0) {
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

	h->gpio_reset = gpio_open(gpio_reset_unit);
	h->gpio_reset_pin = gpio_reset_pin;
	if (h->gpio_reset == GPIO_INVALID_HANDLE) {
		free(h);
		close(h->spi_fd);
		return (SSD1306_INVALID_HANDLE);
	}

	if (gpio_pin_output(h->gpio_reset, gpio_reset_pin)) {
		close(h->spi_fd);
		gpio_close(h->gpio_reset);
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

	h->gpio_dc = gpio_open(gpio_dc_unit);
	h->gpio_dc_pin = gpio_dc_pin;
	if (h->gpio_dc == GPIO_INVALID_HANDLE) {
		close(h->spi_fd);
		gpio_close(h->gpio_reset);
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

	if (gpio_pin_output(h->gpio_dc, gpio_dc_pin)) {
		close(h->spi_fd);
		gpio_close(h->gpio_dc);
		gpio_close(h->gpio_reset);
		free(h);
		return (SSD1306_INVALID_HANDLE);
	}

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
	close(h->spi_fd);
	gpio_close(h->gpio_reset);
	gpio_close(h->gpio_dc);
	free(h);
}

int
ssd1306_initialize(ssd1306_handle_t h)
{
	switch (h->model) {
	case SSD1306_MODEL_128X32:
		ssd1306_initialize_128x32(h);
		break;
	case SSD1306_MODEL_96X16: /* not supported yet */
	case SSD1306_MODEL_128X64: /* not supported yet */
	default:
		return (-1);
	}
	return (0);
}
