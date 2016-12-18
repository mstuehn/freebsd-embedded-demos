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
