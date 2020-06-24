#!/bin/sh
#
# PROVIDE: info_screen
# REQUIRE: networking
# KEYWORD:

. /etc/rc.subr

name="info_screen"
rcvar="info_screen_enable"
info_screen_user="info_screen"
info_screen_command="/usr/local/bin/info_screen"
pidfile="/var/run/info_screen/${name}.pid"
command="/usr/sbin/daemon"
command_args="-P ${pidfile} -r -f ${info_screen_command}"

load_rc_config $name
: ${info_screen_enable:=no}

run_rc_command "$1"
