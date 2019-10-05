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
#include <sys/resource.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include "ssd1306.h"

/* My Raspberry Pi setup */
#define	IICDEV "/dev/iic0"
#define	MODEL	SSD1306_MODEL_128X32

void usage(const char *prog)
{
	fprintf(stderr, "%s: [-f /dev/iicN] [-a addr]\n", prog);
	fprintf(stderr, "\t-F\t\tshow temperature in Fahreheits\n");
}

void
paint_information_screen(ssd1306_handle_t h, int panoffset, int amb, int cpu, struct loadavg* load )
{
	int width, height;
	int font_width, font_height;
	int x, y;
	char str[16] = {0};
	time_t current_time = time (NULL);
	struct tm* local_time = localtime (&current_time);
	int has_cpu;

	if (cpu != INT_MIN) has_cpu = 1;

	cpu = cpu-273;

	ssd1306_clear(h);
	width = ssd1306_width(h);
	height = ssd1306_height(h);
	font_width = ssd1306_font_width(h);
	font_height = ssd1306_font_height(h);

	snprintf(str, sizeof(str), "%0.2f/%0.2f/%0.2f", 
			(double)load->ldavg[0]/load->fscale,
			(double)load->ldavg[1]/load->fscale,
			(double)load->ldavg[2]/load->fscale);
	x = (width - strlen(str) * font_width) / 2;
	y = (height - font_height) / 2;
	y -= panoffset;
	ssd1306_putstr(h, x, y, str);

	if (has_cpu)
		snprintf(str, sizeof(str), "CPU: %d%cC", cpu, '\xf8');
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
	int addr;
	int fahrenheit = 0;
	char scale;
	int cpu_temp, amb_temp;
	int flags;
	int skip;
	size_t oldlen = sizeof(cpu_temp);
	ssd1306_handle_t ssd1306;

	prog = argv[0];
	flags = 0;
	skip = 0;

	while ((ch = getopt(argc, argv, "a:Firs")) != -1) {
		switch (ch) {
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
	struct loadavg load;

	while (1) {
		if (pan == 0) {
			if (sysctlbyname("dev.aw_thermal.0.cpu", &cpu_temp, &oldlen, NULL, 0))
			{
				printf("Failed to read cpu_temp: %s\n", strerror(errno) );
				cpu_temp = INT_MIN;
			} else printf("cpu_temp: %d \n", cpu_temp );

			oldlen = sizeof(load);
			if (sysctlbyname("vm.loadavg", &load, &oldlen, NULL, 0))
			{
				printf("Failed to read cpu_temp: %s\n", strerror(errno) );
				cpu_temp = INT_MIN;
			} else printf("loadavg: %0.4f/%0.4f/%0.4f\n", 
				(double)load.ldavg[0]/load.fscale,
				(double)load.ldavg[1]/load.fscale,
				(double)load.ldavg[2]/load.fscale);
		}

		paint_information_screen(ssd1306, pan, amb_temp, cpu_temp, &load);
		sleep(2);
		for (int i = 0; i < ssd1306_height(ssd1306); i++) {
			pan += dir;
			paint_information_screen(ssd1306, pan, amb_temp, cpu_temp, &load);
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
