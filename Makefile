CC:=gcc
CFLAGS:=-Wall -O2 -pipe -pedantic

prefix:=/usr/local
confdir:=/etc/badge_daemon
wwwdir:=/var/www
dbfile:=/var/lib/badge_daemon/citofonoweb.db

PROGRAMS:=hid_read door_open badge_daemon

OPTIONS:=-Dlights
LIBS:=-lsqlite3 -lpthread -L. -ldoor
MACHINE:=placeholder

all: $(PROGRAMS)
.PHONY: all

hid_read: hid_read.c
	$(CC) $(CFLAGS) -DCONFPATH='"$(confdir)/hid_read.conf"' $< -o $@

libdoor.o: libdoor_$(MACHINE).c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libdoor.so: libdoor.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ $<

door_open.o: door_open.c libdoor.so
	if [ -e '/usr/include/json' ]; then \
	    $(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -Djson -ljson $< -c ; \
	else \
	    $(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -ljson-c $< -c ; \
	fi

door_open: door_open.o libdoor.so
	if [ -e '/usr/include/json' ]; then \
	    $(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -Djson -ljson $< -o $@ ; \
	else \
	    $(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -ljson-c $< -o $@ ; \
	fi

badge_daemon.o: badge_daemon.c libdoor.so
	$(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -DCONFPATH='"$(confdir)/badge_daemon.conf"' $< -c

badge_daemon: badge_daemon.o libdoor.so
	$(CC) $(CFLAGS) $(OPTIONS) $(LIBS) -DCONFPATH='"$(confdir)/badge_daemon.conf"' $< -o $@

install: $(PROGRAMS)
	mkdir -p $(prefix)/sbin
	install -m 0755 -t $(prefix)/sbin $^
	
	mkdir -p $(prefix)/lib
	install -m 0755 -t $(prefix)/lib libdoor.so
	ldconfig
	
	mkdir -p $(confdir)
	install -m 0640 conf/hid_read.conf $(confdir)
	install -m 0640 conf/badge_daemon.conf $(confdir)
	sed -i s:'^source \./':'source $(prefix)/sbin/': $(confdir)/badge_daemon.conf
	sed -i s:'^helper \./':'helper $(prefix)/sbin/': $(confdir)/badge_daemon.conf
	sed -i s:'^dbfile citofonoweb.db':'dbfile $(dbfile)': $(confdir)/badge_daemon.conf
	
	mkdir -p `dirname $(dbfile)`
	install -m 0644 resources/db.info `dirname $(dbfile)`
	
	install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon
	if [ `lsb_release -is` = 'Debian' ]; then \
	    install -m 0755 script/debian_initscript /etc/init.d/badge_daemon ; \
	    sed -i s:'^DAEMON="badge_daemon"':'DAEMON="$(prefix)/sbin/badge_daemon': /etc/init.d/badge_daemon
	fi

	mkdir -p $(wwwdir)/
	cp -rf CitofonoWeb $(wwwdir)/
	
	sed -i s:'/etc/badge_daemon':'$(confdir)': $(wwwdir)/CitofonoWeb/config.inc.php
	sed -i s:'/var/lib/citofonoweb/citofonoweb.db':'$(dbfile)': $(wwwdir)/CitofonoWeb/phpliteadmin/phpliteadmin.php
	chmod +x script/db_update.sh
.PHONY: install

db-update:
	script/db_update.sh '$(dbfile)'
.PHONY: db-update

lighttpd-config:
	install -m 0644 examples/lighttpd-plain.user /etc/lighttpd/
	install -m 0644 examples/lighttpd.conf.example /etc/lighttpd/lighttpd.conf
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
	rm -f *.so
	rm -f *.o
.PHONY: clean
