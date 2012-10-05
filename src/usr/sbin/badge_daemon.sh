#!/bin/bash
parser="/usr/sbin/badge_open.php"
pidfile="/var/run/badge_daemon.pid"

#Accendi il led di stato
gpio -g mode 17 out
gpio -g write 17 1

if [ -f "$pidfile" ]; then
	pid=`cat $pidfile`
else
	pid=''
fi

if [ -z "$pid" ]; then
	$parser
else
	if `kill -0 $pid`; then
		echo 'The daemon is already running.'
		exit 1
	else
		$parser
	fi
fi

#Errore! Notifica via mail o altro
#Spegni il led di stato
gpio -g write 17 0

rm -f "$pidfile"

exit 0
