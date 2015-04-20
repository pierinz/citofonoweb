#!/bin/bash
#Save data to remote server through SSH

#Remote server
RHOST="server"
#Remote user
RUSER="user"

#Arguments
tstamp="$1"
badge="$2"

#Get the badge owner
badge_user=`echo "SELECT user FROM citofonoweb.users WHERE rfid_code = '$badge'; " | mysql -u root -bN`

if [ -z "$badge_user" ]; then
	exit 1
fi

#Get the swipe way
# 0: enter
# 1: exit
# 2: autodetect
badge_way='2'

#Remote command to save data
RCOMMAND="save_command '$badge_user' '$tstamp' '$badge_way'"

#Execute remote command
echo $RCOMMAND | ssh $RUSER@$RHOST -i /etc/badge_daemon/.ssh/id_rsa -o UserKnownHostsFile=/etc/badge_daemon/.ssh/known_hosts -o ConnectTimeout=1 -o BatchMode=yes -q

exit $!
