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

ssd1306_handle_t ssd1306_open(const char *spiodev, ssd1306_model model, int gpio_reset_unit,
    int gpio_reset_pin, int gpio_dc_unit, int gpio_dc_pin, int flags);
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
