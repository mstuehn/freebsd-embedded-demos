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
#include <sys/sysctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "ssd1306.h"
#include "tmp102.h"

/* My Raspberry Pi setup */
#define	IICDEV "/dev/iic0"
#define	MODEL	SSD1306_MODEL_128X32

void usage(const char *prog)
{
	fprintf(stderr, "%s: [-f /dev/iicN] [-a addr]\n", prog);
	fprintf(stderr, "\t-a addr\t\tTMP102 address (default 0x48)\n");
	fprintf(stderr, "\t-f /dev/iicN\t\tI2C bus (default iic0)\n");
	fprintf(stderr, "\t-F\t\tshow temperature in Fahreheits\n");
}

void
paint_information_screen(ssd1306_handle_t h, int panoffset, int amb, int cpu, int fahrenheit)
{
	int width, height;
	int font_width, font_height;
	int x, y;
	char str[16];
	time_t current_time = time (NULL);
	struct tm* local_time = localtime (&current_time);
	char scale;
	int has_cpu, has_amb;

	if (amb != INT_MIN)
		has_amb = 1;
	if (cpu != INT_MIN)
		has_cpu = 1;

	if (fahrenheit) {
		amb = amb * 9 / 5 + 32;
		cpu = cpu * 9 / 5 + 32;
		scale = 'F';
	}
	else
		scale = 'C';

	ssd1306_clear(h);
	width = ssd1306_width(h);
	height = ssd1306_height(h);
	font_width = ssd1306_font_width(h);
	font_height = ssd1306_font_height(h);
	if (has_amb)
		snprintf(str, sizeof(str), "AMB: %.1f%c%c", amb/1000., '\xf8', scale);
	else
		snprintf(str, sizeof(str), "AMB: N/A");

	x = (width - strlen(str) * font_width) / 2;
	y = (height - font_height) / 2;
	y -= panoffset;
	ssd1306_putstr(h, x, y, str);

	if (has_cpu)
		snprintf(str, sizeof(str), "CPU: %.1f%c%c", cpu/1000., '\xf8', scale);
	else
		snprintf(str, sizeof(str), "CPU: N/A");
	x = (width - strlen(str) * font_width) / 2;
	y += height;
	ssd1306_putstr(h, x, y, str);

	snprintf(str, sizeof(str), "%02d:%02d",
	    local_time->tm_hour, local_time->tm_min);
	x = (width - strlen(str) * font_width) / 2;
	y += height;
	ssd1306_putstr(h, x, y, str);

	ssd1306_refresh(h);
}

int
main(int argc, char **argv)
{
	int ch;
	const char *prog;
	const char *i2c;
	int addr;
	int fahrenheit = 0;
	char scale;
	int cpu_temp, amb_temp;
	int flags;
	int skip;
	size_t oldlen;
	tmp102_handle_t tmp102;
	ssd1306_handle_t ssd1306;

	prog = argv[0];
	i2c = "/dev/iic0";
	addr = TMP102_DEFAULT_ADDR;
	flags = 0;
	skip = 0;

	while ((ch = getopt(argc, argv, "a:f:Firs")) != -1) {
		switch (ch) {
		case 'f':
			i2c = optarg;
			break;
		case 'a':
			addr = strtol(optarg, NULL, 0);
			break;
		case 'F':
			fahrenheit = 1;
			break;
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

	tmp102 = tmp102_open(i2c, addr);
	if (tmp102 == TMP102_INVALID_HANDLE) {
		fprintf(stderr, "Failed to open TMP102\n");
		return (1);
	}


	ssd1306 = ssd1306_open(IICDEV, MODEL, 0x3C, flags);
	if (ssd1306 == SSD1306_INVALID_HANDLE) {
		fprintf(stderr, "failed to create SSD1306 handle\n");
		return (1);
	}

	if (!skip && ssd1306_initialize(ssd1306)) {
		fprintf(stderr, "failed to initialize SSD1306\n");
		ssd1306_close(ssd1306);
		return (1);
	}

	ssd1306_clear(ssd1306);
	ssd1306_refresh(ssd1306);
	ssd1306_on(ssd1306);

	int pan = 0;
	int dir = 1;
	fahrenheit = 0;
	while (1) {
		if (pan == 0) {
			if (tmp102_read_temp(tmp102, &amb_temp))
				amb_temp = INT_MIN;
			/* Specific to RPi */
			oldlen = sizeof(int);
			if (sysctlbyname("hw.cpufreq.temperature", &cpu_temp, &oldlen, NULL, 0))
				cpu_temp = INT_MIN;
		}

		paint_information_screen(ssd1306, pan, amb_temp, cpu_temp, fahrenheit);
		sleep(2);
		for (int i = 0; i < ssd1306_height(ssd1306); i++) {
			pan += dir;
			paint_information_screen(ssd1306, pan, amb_temp, cpu_temp, fahrenheit);
			usleep(20000);
		}

		if (pan >= 2*ssd1306_height(ssd1306)) {
			pan = 2*ssd1306_height(ssd1306);
			dir = -1;
			fahrenheit = !fahrenheit;
		}

		if (pan <= 0) {
			pan = 0;
			dir = 1;
		}
	}

	return (0);
}
