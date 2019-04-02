#!/sbin/openrc-run

CONFIG=/etc/brubeck/config.json
LOG=/var/log/brubeck.log

name="$SVCNAME"
command="/usr/local/sbin/$SVCNAME"
command_args="--config=$CONFIG --log=$LOG"
pidfile="/var/run/brubeck.pid"
command_background=true

