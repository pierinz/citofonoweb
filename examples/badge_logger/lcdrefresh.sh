#!/bin/bash
# This example show a simple message on the LCD screen
# You should add a cronjob to refresh the time

# The lockfile is needed to prevent other programs to write to the lcd screen concurrently
LOCKFILE="/tmp/lcd.lock"

msg=`mktemp`
echo "`date +%d/%m/%Y`     `date +%H:%M`" > $msg
echo >> $msg
echo "    SWIPE BADGE" >> $msg
echo >> $msg

# Test if server is offline
#REMOTE=http://test.loc
if [ x"$REMOTE" != 'x' ]; then
	wget -q --tries=1 --timeout=0.1 --spider $REMOTE
	if [ $? -ne 0 ]; then
			echo "     (OFF-LINE)" >> $msg
	fi
fi

# Ensure nobody is writing
if [ ! -e "$LOCKFILE" ]; then
	cat  $msg | lcdscreen
fi
rm $msg
exit 0
