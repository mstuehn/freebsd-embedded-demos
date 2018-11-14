/*-
 * Copyright (c) 2018 Oleksandr Tymoshenko <gonzo@bluezbox.com>
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

/*
 * Based on https://github.com/pimoroni/inky
 */

#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgpio.h>
#include <string.h>
#include <sys/spigenio.h>
#include <stdio.h>
#include <time.h>

#include "luts.h"

#define	GPIO_UNIT	0
/* SPI0 CS0 */
#define SPIDEV		"/dev/spigen0.0"

#define	GPIO_RESET_PIN	27
#define	GPIO_DC_PIN	22
#define	GPIO_BUSY_PIN	17

#define	PHAT_WIDTH	212
#define	PHAT_HEIGHT	104

/* one-quarter of the ornament square */
#define	QUARTER_SIZE	13

#define	INKY_INVALID_HANDLE	0

#define	COLOR_BLACK	0x0
#define	COLOR_RED	0x1
#define	COLOR_WHITE	0x2

struct inky_handle {
	int		spi_fd;

	gpio_handle_t	gpio;

	int		width;
	int		height;
	int		screen_size;
	uint8_t		*screen; /* byte-per-pixel screen buffer */
	int		scratch_size;
	/* Compressed bit-per pixel buffers */
	uint8_t		*scratch_colorw; /* B/W */
	uint8_t		*scratch_color; /* Red/Yellow */
};

typedef struct inky_handle *inky_handle_t;

static void
inky_busy_wait(inky_handle_t h)
{
	int v = gpio_pin_get(h->gpio, GPIO_BUSY_PIN);
	while (gpio_pin_get(h->gpio, GPIO_BUSY_PIN) != GPIO_VALUE_LOW)
		usleep(10000);
}

int
inky_spi_transfer(inky_handle_t h, uint8_t *data, int len)
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
	if (ioctl(h->spi_fd, SPIGENIOC_TRANSFER, &transfer) < 0) {
		fprintf(stderr, "SPIO transfare failed\n");
		return (-1);
	}

	return (0);
}

static int
inky_command(inky_handle_t h, uint8_t cmd)
{
	if (gpio_pin_low(h->gpio, GPIO_DC_PIN))
		return (-1);
	if (inky_spi_transfer(h, &cmd, 1))
		return (-1);

	return (0);
}

static int
inky_data(inky_handle_t h, uint8_t *data, int len)
{
	if (gpio_pin_high(h->gpio, GPIO_DC_PIN))
		return (-1);
	if (inky_spi_transfer(h, data, len))
		return (-1);

	return (0);
}

static int
inky_command_with_data(inky_handle_t h, uint8_t cmd, uint8_t data)
{
	if (inky_command(h, cmd) != 0)
		return (-1);

	return inky_data(h, &data, 1);
}

inky_handle_t
inky_open()
{
	inky_handle_t h;
	h = malloc(sizeof *h);

	h->height = PHAT_WIDTH;
	h->width = PHAT_HEIGHT;

	h->spi_fd = open(SPIDEV, O_RDWR);
	if (h->spi_fd < 0) {
		fprintf(stderr, "failed to open SPI device %s\n", SPIDEV);
		free(h);
		return (INKY_INVALID_HANDLE);
	}

	h->gpio = gpio_open(GPIO_UNIT);
	if (h->gpio == GPIO_INVALID_HANDLE) {
		fprintf(stderr, "failed to open GPIO unit %d\n", GPIO_UNIT);
		free(h);
		close(h->spi_fd);
		return (INKY_INVALID_HANDLE);
	}

	if (gpio_pin_output(h->gpio, GPIO_RESET_PIN)) {
		fprintf(stderr, "failed to configure reset pin\n");
		close(h->spi_fd);
		gpio_close(h->gpio);
		free(h);
		return (INKY_INVALID_HANDLE);
	}

	if (gpio_pin_output(h->gpio, GPIO_DC_PIN)) {
		fprintf(stderr, "failed to configure data/command pin\n");
		close(h->spi_fd);
		gpio_close(h->gpio);
		free(h);
		return (INKY_INVALID_HANDLE);
	}

	if (gpio_pin_input(h->gpio, GPIO_BUSY_PIN)) {
		fprintf(stderr, "failed to configure busy pin\n");
		close(h->spi_fd);
		gpio_close(h->gpio);
		free(h);
		return (INKY_INVALID_HANDLE);
	}

	h->screen_size = h->width * h->height;
	h->screen = malloc(h->screen_size);
	h->scratch_size = h->width * h->height / 8;
	h->scratch_colorw = malloc(h->scratch_size);
	h->scratch_color = malloc(h->scratch_size);
	memset(h->screen, COLOR_WHITE, h->screen_size);

	gpio_pin_low(h->gpio, GPIO_DC_PIN);

	return (h);
}

/*
 * Reset the device
 */
static void
inky_reset(inky_handle_t h)
{
	/* reset */
	gpio_pin_low(h->gpio, GPIO_RESET_PIN);
	usleep(100000);
	gpio_pin_high(h->gpio, GPIO_RESET_PIN);
	usleep(100000);

	inky_command(h, 0x12); /* Soft reset */
	inky_busy_wait(h);
}

/*
 * Update the device with the content of the screen buffer
 */
static void
inky_update(inky_handle_t h)
{
	/* temporary data buffer */
	uint8_t data[16];
	int x, y;
	int sx, sy;
	uint8_t *page, *scratch;
	int bit, onoff, color;

	memset(h->scratch_colorw, 0xff, h->scratch_size);
	memset(h->scratch_color, 0x0, h->scratch_size);

	for (x = 0; x < h->width; x++) {
		for (y = 0; y < h->height; y++) {
			sx = x;
			sy = y;
			color = h->screen[y * h->width + x];
			if (color == COLOR_BLACK) {
				scratch = h->scratch_colorw;
				onoff = 0;
			} else if (color == COLOR_RED) {
				scratch = h->scratch_color;
				onoff = 1;
			}
			else
				continue;
			page = scratch + ((sy*h->width + sx)/8);
			bit = 7 - sx % 8;
			if (onoff)
				*page |= (1 << bit);
			else
				*page &= ~(1 << bit);
		}
	}

	inky_reset(h);

	inky_command_with_data(h, 0x74, 0x54); /* Set Analog Block Control */
	inky_command_with_data(h, 0x7e, 0x3b); /* Set Digital Block Control */

	/* Gate setting */
	data[0] = h->height % 256;
	data[1] = h->height / 256;
	data[2] = 0;
	inky_command(h, 0x01);
	inky_data(h, data, 3);

	/* Gate Driving Voltage */
	data[0] = 0b10000;
	data[1] = 0b0001;
	inky_command(h, 0x03);
	inky_data(h, data, 2);

	inky_command_with_data(h, 0x3a, 0x07); /* Dummy line period */
	inky_command_with_data(h, 0x3b, 0x04); /* Gate line width */
	inky_command_with_data(h, 0x11, 0x03); /* Data entry mode setting 0x03 = X/Y increment */

	inky_command(h, 0x04);  /* Power On */
	inky_command_with_data(h, 0x2c, 0x3c);  /* VCOM Register, 0x3c = -1.5v? */

	inky_command_with_data(h, 0x3c, 0x00);
	/* 0x00 - Black, 0x33 - Yellow/Red, 0xFF - White */
	inky_command_with_data(h, 0x3c, 0xFF);

#if 0
	/* Required for yellow */
	inky_command_with_data(h, 0x04, 0x07);  /* Set voltage of VSH and VSL */
#endif

	/* Set LUTs */
	inky_command(h, 0x32);
	inky_data(h, RED_LUTS, sizeof(RED_LUTS));  /* Set LUTs */

	/* Set RAM X Start/End */
	data[0] = 0;
	data[1] = h->width/8 - 1;
	inky_command(h, 0x44);
	inky_data(h, data, 2);

	/* Set RAM Y Start/End */
	data[0] = 0;
	data[1] = 0;
	data[2] = h->height % 256;
	data[3] = h->height / 256;
	inky_command(h, 0x45);
	inky_data(h, data, 4);

	/* Black and White part */
	inky_command_with_data(h, 0x4e, 0x00);  /* Set RAM X Pointer Start */
	data[1] = data[0] = 0;
	inky_command(h, 0x4f);  /* Set RAM Y Pointer Start */
	inky_data(h, data, 2); 
	inky_command(h, 0x24);
	inky_data(h, h->scratch_colorw, h->scratch_size);

	/* Red part */
	inky_command_with_data(h, 0x4e, 0x00);  /* Set RAM X Pointer Start */
	data[1] = data[0] = 0;
	inky_command(h, 0x4f);  /* Set RAM Y Pointer Start */
	inky_data(h, data, 2); 
	inky_command(h, 0x26);
	inky_data(h, h->scratch_color, h->scratch_size);

	inky_command_with_data(h, 0x22, 0xc7);  /* Display Update Sequence */
	inky_command(h, 0x20);  /* Trigger Display Update */
	usleep(50000);
	inky_busy_wait(h);
	inky_command_with_data(h, 0x10, 0x01);  /* Enter Deep Sleep */
}

/*
 * Put one pixel at (x, y)
 */
static int
inky_put_pixel(inky_handle_t h, int x, int y, int color)
{
	int sx, sy;

	/* Rotate by -90 degrees */
	sx = h->width - y;
	sy = x;

	if ((sx < 0) || (sx >= h->width))
		return (-1);

	if ((sy < 0) || (sy >= h->height))
		return (-1);

	h->screen[sy * h->width + sx] = color;
	return (0);
}

/*
 * Fill whole screen with the provided color
 */
static void
inky_fill(inky_handle_t h, uint8_t color)
{
	memset(h->screen, color, h->screen_size);
}

static void
inky_close(inky_handle_t h)
{
	free(h->screen);
	close(h->spi_fd);
	gpio_close(h->gpio);
	free(h);
}

int main(int argc, const char *argv[])
{
	
	inky_handle_t inky;
	int x, y;

	printf("INKY PHAT demo\n");
	inky = inky_open();
	if (inky == INKY_INVALID_HANDLE) {
		fprintf(stderr, "failed to open INKY device\n");
		exit(1);
	}

	inky_fill(inky, COLOR_WHITE);

	/* Good enough for ornaments */
	srandom(time(NULL) + getpid());

	/* Create random ornament */
	for (int stampy = 0; stampy < PHAT_HEIGHT; stampy += QUARTER_SIZE*2 + 2) {
		for (x = 0; x < QUARTER_SIZE; x++) {
			for (y = 0; y < QUARTER_SIZE; y++) {
				uint8_t color;
				/* Biased but, again, good enough for ornaments */
				uint32_t r = random() % 5;
				if (r < 2)
					color = COLOR_RED;
				else if (r < 4)
					color = COLOR_BLACK;
				else
					color = COLOR_WHITE;

				for (int stampx = 0; stampx < PHAT_WIDTH; stampx += QUARTER_SIZE*2) {
					inky_put_pixel(inky, stampx + x, stampy + y, color);
					inky_put_pixel(inky, stampx + QUARTER_SIZE*2 - 1 - x, stampy + y, color);
					inky_put_pixel(inky, stampx + QUARTER_SIZE*2 - 1 - x, stampy + QUARTER_SIZE*2 - 1 - y, color);
					inky_put_pixel(inky, stampx + x, stampy + QUARTER_SIZE*2 - 1 - y, color);
				}
			}
		}
	}

	inky_reset(inky);
	inky_update(inky);
	inky_close(inky);

	return 0;
}
