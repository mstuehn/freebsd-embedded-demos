#ifndef	LUTS_H
#define	LUTS_H

static uint8_t BLACK_LUTS[] = {
	/* Phase 0     Phase 1     Phase 2     Phase 3     Phase 4     Phase 5     Phase 6 */
	/* A B C D     A B C D     A B C D     A B C D     A B C D     A B C D     A B C D */
	0b01001000, 0b10100000, 0b00010000, 0b00010000, 0b00010011, 0b00000000, 0b00000000,  /* LUT0 - Black */
	0b01001000, 0b10100000, 0b10000000, 0b00000000, 0b00000011, 0b00000000, 0b00000000,  /* LUTT1 - White */
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  /* IGNORE */
	0b01001000, 0b10100101, 0b00000000, 0b10111011, 0b00000000, 0b00000000, 0b00000000,  /* LUT3 - Red */
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  /* LUT4 - VCOM */

	/* Duration	    |  Repeat */
	/* A   B     C     D   | */
	16,   4,    4,    4,     4,   /* 0 Flash */
	16,   4,    4,    4,     4,   /* 1 clear */
	4,    8,    8,    16,    16,  /* 2 bring in the black */
	0,    0,    0,    0,     0,   /* 3 time for red */
	0,    0,    0,    0,     0,   /* 4 final black sharpen phase */
	0,    0,    0,    0,     0,   /* 5 */
	0,    0,    0,    0,     0,   /* 6 */
};

static uint8_t RED_LUTS[] = {
	    /* Phase 0     Phase 1     Phase 2     Phase 3     Phase 4     Phase 5     Phase 6 */
	    /* A B C D     A B C D     A B C D     A B C D     A B C D     A B C D     A B C D */
	    0b01001000, 0b10100000, 0b00010000, 0b00010000, 0b00010011, 0b00000000, 0b00000000,  /* LUT0 - Black */
	    0b01001000, 0b10100000, 0b10000000, 0b00000000, 0b00000011, 0b00000000, 0b00000000,  /* LUTT1 - White */
	    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  /* IGNORE */
	    0b01001000, 0b10100101, 0b00000000, 0b10111011, 0b00000000, 0b00000000, 0b00000000,  /* LUT3 - Red */
	    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  /* LUT4 - VCOM */

	    /* Duration	    |  Repeat */
	    /* A   B     C     D   | */
	    64,   12,   32,   12,    6,   /* 0 Flash */
	    16,   8,    4,    4,     6,   /* 1 clear */
	    4,    8,    8,    16,    16,  /* 2 bring in the black */
	    2,    2,    2,    64,    32,  /* 3 time for red */
	    2,    2,    2,    2,     2,   /* 4 final black sharpen phase */
	    0,    0,    0,    0,     0,   /* 5 */
	    0,    0,    0,    0,     0    /* 6 */
};

static uint8_t YELLOW_LUTS[] = {
	/* Phase 0     Phase 1     Phase 2     Phase 3     Phase 4     Phase 5     Phase 6 */
	/* A B C D     A B C D     A B C D     A B C D     A B C D     A B C D     A B C D */
	0b11111010, 0b10010100, 0b10001100, 0b11000000, 0b11010000,  0b00000000, 0b00000000,  /* LUT0 - Black */
	0b11111010, 0b10010100, 0b00101100, 0b10000000, 0b11100000,  0b00000000, 0b00000000,  /* LUTT1 - White */
	0b11111010, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  0b00000000, 0b00000000,  /* IGNORE */
	0b11111010, 0b10010100, 0b11111000, 0b10000000, 0b01010000,  0b00000000, 0b11001100,  /* LUT3 - Yellow (or Red) */
	0b10111111, 0b01011000, 0b11111100, 0b10000000, 0b11010000,  0b00000000, 0b00010001,  /* LUT4 - VCOM */

	/* Duration	    | Repeat */
	/* A   B     C     D   | */
	64,   16,   64,   16,   8,
	8,    16,   4,    4,    16,
	8,    8,    3,    8,    32,
	8,    4,    0,    0,    16,
	16,   8,    8,    0,    32,
	0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,
};

#endif	/* LUTS_H */
