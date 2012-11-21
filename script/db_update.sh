#!/bin/bash
#Update DB to the latest version
dbfile="$1"

#If database doesn't exist, the user needs to configure the software and generate one later 
if [ -f "$1" ]; then
	#Try to add any missing field
	echo 'alter table acl add column description varchar(50);' | sqlite3 $1 2>/dev/null
fi

exit 0