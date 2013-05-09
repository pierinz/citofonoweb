CC:=gcc

prefix:=/usr/local
confdir:=/etc/badge_daemon
wwwdir:=/var/www
dbfile:=/var/lib/badge_daemon/citofonoweb.db

CFLAGS:=-Wall -O2 -pipe
PROGRAMS:=hid_read door_open badge_daemon

OPTIONS:=-Dlights -DCONFDIR'"$(confdir)"'
MACHINE:=placeholder

all: $(PROGRAMS)
.PHONY: all

hid_read: hid_read.c
	$(CC) $(CFLAGS) $< -o $@

door_lib.o: door_lib_$(MACHINE).c
	$(CC) $(CFLAGS) -c $< -o $@

door_open.o: door_open.c
	if [ -e '/usr/include/json' ]; then \
	    $(CC) $(CFLAGS) -Djson -lsqlite3 -ljson $< ; \
	else \
	    $(CC) $(CFLAGS) -lsqlite3 -ljson-c -c $< ; \
	fi

door_open: door_lib.o door_open.o
	$(CC) $(CFLAGS) -lsqlite3 -ljson-c $^ -o $@

badge_daemon.o: badge_daemon.c

badge_daemon: badge_daemon.o door_lib.o
	$(CC) $(CFLAGS) $(OPTIONS) -lpthread $^ -o $@

install: $(PROGRAMS)
	mkdir -p $(prefix)/sbin
	install -m 0755 $(prefix)/sbin $^
	
	install -m 0640 conf/hid_read.conf $(confdir)
	install -m 0640 conf/badge_daemon.conf $(confdir)
	sed -i s:'^source \./':'source $(prefix)/sbin': $(confdir)/badge_daemon.conf
	sed -i s:'^helper \./':'helper $(prefix)/sbin': $(confdir)/badge_daemon.conf
	sed -i s:'^dbfile citofonoweb.db':'dbfile $(dbfile)': $(confdir)/badge_daemon.conf
	
	mkdir -p `dirname $(dbfile)`
	install -m 0644 resources/db.info `dirname $(dbfile)`
	
	install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon

	mkdir -p $(wwwdir)/
	cp -rf CitofonoWeb $(wwwdir)/
	
	sed -i s:'/var/lib/citofonoweb/citofonoweb.db':'$(dbfile)': $(wwwdir)/CitofonoWeb/phpliteadmin/phpliteadmin.php
	chmod +x script/db_update.sh
.PHONY: install

db-update:
	script/db_update.sh '$(dbfile)'
.PHONY: db-update

lighttpd-config:
	install -m 0644 examples/lighttpd-plain.user /etc/lighttpd/
	install -m 0644 examples/lighttp.conf.example /etc/lighttpd/lighttp.conf
	lighttpd-enable-mod fastcgi-php auth
	/etc/init.d/lighttpd restart
.PHONY: db-update

uninstall:
	rm -f $(prefix)/sbin/badge_daemon
	rm -f $(prefix)/sbin/hid_read
	rm -f $(prefix)/sbin/door_open

	rm -f /etc/init.d/badge_daemon
	rm -f /etc/logrotate.d/badge_daemon
	rm -rf $(wwwdir)/CitofonoWeb
.PHONY: uninstall

clean:
	rm -f $(PROGRAMS)
	rm -f *.o
.PHONY: clean
