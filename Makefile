SUBDIR= libtmp102 libssd1306
SUBDIR+= ssd1306_progress ssd1306_message tmp102_info info_screen
SUBDIR+= inky_demo

.include <bsd.arch.inc.mk>
SUBDIR_PARALLEL=
.include <bsd.subdir.mk>
