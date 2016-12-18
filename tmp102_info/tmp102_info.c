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
#include <limits.h>
#include "tmp102.h"

void usage(const char *prog)
{
	fprintf(stderr, "%s: [-f /dev/iicN] [-a addr]\n", prog);
	fprintf(stderr, "\t-a addr\t\tTMP102 address (default 0x48)\n");
	fprintf(stderr, "\t-f /dev/iicN\t\tI2C bus (default iic0)\n");
	fprintf(stderr, "\t-F\t\tshow temperature in Fahreheits\n");
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
	int temp;
	tmp102_handle_t tmp102;

	prog = argv[0];
	i2c = "/dev/iic0";
	addr = TMP102_DEFAULT_ADDR;

	while ((ch = getopt(argc, argv, "a:f:F")) != -1) {
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

	if (tmp102_read_temp(tmp102, &temp)) {
		fprintf(stderr, "Failed to read tempreture from TMP102\n");
		tmp102_close(tmp102);
		return (1);
	}

	if (fahrenheit) {
		scale = 'F';
		temp = temp * 9 / 5 + 32;
	}
	else
		scale = 'C';

	printf("Temperature is %.1f %c\n", temp/1000., scale);

	tmp102_close(tmp102);
	return (0);
}
