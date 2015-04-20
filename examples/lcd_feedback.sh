#!/bin/bash
# This example shows how to write the user name on a LCD screen, given the badge code

# The lockfile is needed to prevent other programs to write to the lcd screen concurrently
LOCKFILE="/tmp/lcd.lock"
badge="$1"

# The monitor shouldn't be refreshed now
touch $LOCKFILE

# Get the badge owner
badge_user=`echo "SELECT user FROM citofonoweb.users WHERE rfid_code = '$badge'; " | mysql -u root -bN`

msg=`mktemp`
echo "`date +%d/%m/%Y`     `date +%H:%M`" > $msg
echo >> $msg
echo "     $badge_user" >> $msg
echo "  BADGE REGISTERED" >> $msg
cat $msg | lcdscreen

# Do you need some noise?
#buzzer -g 26 -f 1400 -d 1

sleep 2

echo "`date +%d/%m/%Y`     `date +%H:%M`" > $msg
echo >> $msg
echo "    SWIPE BADGE" >> $msg
echo >> $msg
cat  $msg | lcdscreen

rm $msg

rm $LOCKFILE
exit 0
