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

#ifndef __SSD1306_H__
#define __SSD1306_H__

typedef enum {
	SSD1306_EXTERNALVCC,
	SSD1306_SWITCHCAPVCC
} ssd1306_vccstate;

typedef enum {
	SSD1306_MODEL_96X16,
	SSD1306_MODEL_128X32,
	SSD1306_MODEL_128X64
} ssd1306_model;

typedef enum {
	SSD1306_FONT_8,
	SSD1306_FONT_14,
	SSD1306_FONT_16
} ssd1306_font;

#define	SSD1306_INVALID_HANDLE	NULL

#define	SSD1306_FLAG_INVERSE	(1 << 0)
#define	SSD1306_FLAG_ROTATE	(1 << 1)

typedef struct ssd1306_handle* ssd1306_handle_t;

ssd1306_handle_t ssd1306_open(const char *spiodev, ssd1306_model model, uint8_t slave_address, int flags);
void ssd1306_close(ssd1306_handle_t);
int ssd1306_initialize(ssd1306_handle_t h);
int ssd1306_on(ssd1306_handle_t h);
int ssd1306_off(ssd1306_handle_t h);
int ssd1306_refresh(ssd1306_handle_t h);
int ssd1306_width(ssd1306_handle_t h);
int ssd1306_height(ssd1306_handle_t h);
int ssd1306_font_width(ssd1306_handle_t h);
int ssd1306_font_height(ssd1306_handle_t h);
void ssd1306_clear(ssd1306_handle_t h);
void ssd1306_putpixel(ssd1306_handle_t h, int x, int y, int v);
void ssd1306_putchar(ssd1306_handle_t h, int x, int y, unsigned char);
void ssd1306_putstr(ssd1306_handle_t h, int x, int y, const char *s);

#endif /* __SSD1306_H__ */
