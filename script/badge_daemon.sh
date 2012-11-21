#!/bin/bash
parser="`dirname $0`/badge_open.php"
pidfile="/var/run/badge_daemon.pid"
webpath="/var/www"

ledpin=`grep statusled "$webpath/CitofonoWeb/config.inc.php" | egrep -o '[0-9]+'`
logfile=`grep statusled "$webpath/CitofonoWeb/config.inc.php" | egrep -o '[0-9]+'`
version='CitofonoWeb: 0.1'

usage()
{
cat << EOF
usage: $0 [-v] [-V] [-h]

OPTIONS:
        -v              be verbose
        -V				show version
        -h              list available options (this page) 
EOF
}

VERBOSE=

while getopts "hqr" OPTION; do
case $OPTION in
    h)
            usage
            exit 1
            ;;
    v)
            VERBOSE="-v"
            ;;
    V)
            echo $version
            exit 0
            ;;
    ?)
            usage
            exit 1
            ;;
esac
done

if [ $UID -gt 0 ]; then
	echo "You need root privileges to run this program"
	exit 1
fi

#Light up the status led
gpio -g mode $ledpin out
gpio -g write $ledpin 1

if [ -f "$pidfile" ]; then
	pid=`cat $pidfile`
else
	pid=''
fi

if [ -z "$pid" ]; then
	$parser $VERBOSE 2> $logfile
else
	if `kill -0 $pid`; then
		echo 'The daemon is already running.'
		exit 1
	else
		$parser $VERBOSE 2> $logfile
	fi
fi

#Turn off the status led
gpio -g write $ledpin 0

rm -f "$pidfile"

exit 0
