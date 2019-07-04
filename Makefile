SUBDIR= libssd1306
SUBDIR+= ssd1306_progress ssd1306_message info_screen

.include <bsd.arch.inc.mk>
SUBDIR_PARALLEL=
.include <bsd.subdir.mk>
