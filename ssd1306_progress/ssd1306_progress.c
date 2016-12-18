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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ssd1306.h"

/* My Raspberry Pi setup */
#define	SPIDEV	"/dev/spigen0"
#define	GPIOC	0
#define	PIN_DC	23
#define	PIN_RST	24
#define	MODEL	SSD1306_MODEL_128X32

void usage(const char *prog)
{
	fprintf(stderr, "%s: [-irs] msg\n", prog);
	fprintf(stderr, "\t-i\t\tinverse screen\n");
	fprintf(stderr, "\t-r\t\trotate screen by 180\n");
	fprintf(stderr, "\t-s\t\tskip initialization\n");
}

int
main(int argc, char **argv)
{
	ssd1306_handle_t ssd1306;
	const char *msg;
	int flags;
	int ch;
	int width, height;
	int font_width, font_height;
	const char *prog;
	int skip;
	int x, y, c;
	int chars, percent, pos;

	prog = argv[0];

	msg = NULL;
	flags = 0;
	skip = 0;
	while ((ch = getopt(argc, argv, "irs")) != -1) {
		switch (ch) {
		case 'i':
			flags |= SSD1306_FLAG_INVERSE;
			break;
		case 'r':
			flags |= SSD1306_FLAG_ROTATE;
			break;
		case 's':
			skip = 1;
			break;

		case '?':
		default:
			usage(prog);
			return (1);
	     }
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage(prog);
		return (1);
	}

	msg = argv[0];

	ssd1306 = ssd1306_open(SPIDEV, MODEL, GPIOC, PIN_RST, GPIOC, PIN_DC, flags);
	if (ssd1306 == SSD1306_INVALID_HANDLE) {
		fprintf(stderr, "failed to create SSD1306 handle\n");
		return (1);
	}

	if (!skip && ssd1306_initialize(ssd1306)) {
		fprintf(stderr, "failed to initialize SSD1306\n");
		ssd1306_close(ssd1306);
		return (1);
	}

	width = ssd1306_width(ssd1306);
	height = ssd1306_height(ssd1306);
	font_width = ssd1306_font_width(ssd1306);
	font_height = ssd1306_font_height(ssd1306);

	ssd1306_clear(ssd1306);
	ssd1306_refresh(ssd1306);
	ssd1306_on(ssd1306);

	y = (height - font_height * 2) / 2;
	x = (width - strlen(msg) * font_width) / 2;

	chars = width / font_width;
	for (percent = 0; percent <= 100; percent += 10) {
		ssd1306_putstr(ssd1306, x, y, msg);
		/* Position of rightmost "finished" char */
		pos = percent * chars /100;
		for (c = 0; c < chars; c++)
			ssd1306_putchar(ssd1306, font_width * c, y + font_height, c <= pos ? 0xdb : 0xb0);
		ssd1306_refresh(ssd1306);
		usleep(500000);
	}

	ssd1306_close(ssd1306);
	return (0);
}
