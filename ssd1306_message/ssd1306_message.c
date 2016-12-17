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
	fprintf(stderr, "%s: [-irs] msg1 [msg2]\n", prog);
	fprintf(stderr, "\t-i\t\tinverse screen\n");
	fprintf(stderr, "\t-r\t\trotate screen by 180\n");
	fprintf(stderr, "\t-s\t\tskip initialization\n");
}

int
main(int argc, char **argv)
{
	ssd1306_handle_t ssd1306;
	const char *msg1, *msg2;
	int flags;
	int ch;
	int width, height;
	int font_width, font_height;
	const char *prog;
	int skip;
	int x, y;

	prog = argv[0];

	msg1 = msg2 = NULL;
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

	msg1 = argv[0];
	if (argc > 1)
		msg2 = argv[1];

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

	if (msg2) 
		y = (height - font_height * 2) / 2;
	else
		y = (height - font_height) / 2;
	
	x = (width - strlen(msg1) * font_width) / 2;

	ssd1306_putstr(ssd1306, x, y, msg1);
	
	if (msg2) {
		y += font_height;
		x = (width - strlen(msg2) * font_width) / 2;
		ssd1306_putstr(ssd1306, x, y, msg2);
	}

	ssd1306_refresh(ssd1306);
	ssd1306_on(ssd1306);

	ssd1306_close(ssd1306);
	return (0);
}
