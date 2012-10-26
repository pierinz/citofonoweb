#!/bin/bash
parser="`dirname($0)`/badge_open.php"
pidfile="/var/run/badge_daemon.pid"
webpath="/var/www"

ledpin=`grep statusled "$webpath/CitofonoWeb/config.inc.php" | egrep -o '[0-9]+'`

#Accendi il led di stato
gpio -g mode $ledpin out
gpio -g write $ledpin 1

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
gpio -g write $ledpin 0

rm -f "$pidfile"

exit 0
