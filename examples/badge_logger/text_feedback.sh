#!/bin/bash
# This example shows how to write the user name on stdout, given the badge code
badge="$1"

#Get the badge owner
badge_user=`echo "SELECT user FROM citofonoweb.users WHERE rfid_code = '$badge'; " | mysql -u root -bN`

echo $badge_user 

exit 0
