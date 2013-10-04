CC:=gcc
CFLAGS:=-g -pipe -Wall

prefix:=/usr/local
confdir:=/etc/badge_daemon
wwwdir:=/var/www
dbfile:=/var/lib/badge_daemon/citofonoweb.db

BACKEND:=sqlite
PROGRAMS:=hid_read serial_read door_open badge_daemon

ifeq ("$(BACKEND)","mysql")
    LIBS:=-DMYSQL_B `mysql_config --cflags --libs` -lpthread
else
    LIBS:=-DSQLITE_B -lsqlite3 -lpthread -ldl
endif

LIBDOOR:=libdoor_debug.so libdoor_raspberry_sysfs.so
#libdoor_raspberry_piface.so

all: $(PROGRAMS)
.PHONY: all

libdoor.so: $(LIBDOOR)
	if [ -n "`echo $(LIBDOOR) | grep piface`" ]; then \
	    sed -i s/'^blacklist spi\-bcm2708'/'#blacklist spi-bcm2708'/ /etc/modprobe.d/raspi-blacklist.conf 2>/dev/null ; \
	fi
.PHONY: libdoor.so

libdoor_debug.so: libdoor_debug.c
	$(CC) $(CFLAGS) -shared -fPIC -Wl,-soname,$@ -o $@ $<

libdoor_raspberry_piface.so: libdoor_raspberry_piface.c
	$(CC) $(CFLAGS) -shared -fPIC -Wl,-soname,$@  -L/usr/local/lib/ -lpiface-1.0 -o $@ $< || echo "You need to install this library: https://github.com/thomasmacpherson/piface"

libdoor_raspberry_sysfs.so: libdoor_raspberry_sysfs.c
	$(CC) $(CFLAGS) -shared -fPIC -Wl,-soname,$@  -o $@ $<

libdoor_raspberry_gpio.so: libdoor_raspberry_gpio.c
	$(CC) $(CFLAGS) -shared -fPIC -Wl,-soname,$@  -o $@ $<

door_open.o: door_open.c libdoor.so
	if [ -e '/usr/include/json' ]; then \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -Djson -ljson $< -c ; \
	else \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -ljson-c $< -c ; \
	fi

door_open: door_open.o libdoor.so
	if [ -e '/usr/include/json' ]; then \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -Djson -ljson $< -o $@ ; \
	else \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)/badge_daemon.conf"' -ljson-c $< -o $@ ; \
	fi

badge_daemon.o: badge_daemon.c
	$(CC) $(CFLAGS) $(LIBS) -DCONFPATH='"$(confdir)/badge_daemon.conf"' $< -c

badge_daemon: badge_daemon.o
	$(CC) $(CFLAGS) $(LIBS) -DCONFPATH='"$(confdir)/badge_daemon.conf"' $< -o $@

install: $(PROGRAMS)
	mkdir -p $(prefix)/sbin
	install -m 0755 -t $(prefix)/sbin $^
	
	mkdir -p $(prefix)/lib
	install -m 0755 -t $(prefix)/lib libdoor*.so
	ldconfig
	
	mkdir -p $(confdir)
	if [ -e $(confdir)/badge_daemon.conf ]; then \
	    echo "badge_daemon.conf found - skipping" ; \
	    echo "Run 'make conf' to overwrite" ; \
	else \
	    install -m 0640 conf/badge_daemon.conf $(confdir) ; \
	fi
	
	sed -i s:'^source \./':'source $(prefix)/sbin/': $(confdir)/badge_daemon.conf
	sed -i s:'^helper \./':'helper $(prefix)/sbin/': $(confdir)/badge_daemon.conf
	sed -i s:'^dbfile citofonoweb.db':'dbfile $(dbfile)': $(confdir)/badge_daemon.conf
	sed -i s:'^libdoor ./libdoor_debug.so':'libdoor $(prefix)/lib/libdoor_debug.so': $(confdir)/badge_daemon.conf
	
	mkdir -p `dirname $(dbfile)`
	install -m 0644 resources/db.info `dirname $(dbfile)`
	
	install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon
	if [ `lsb_release -is` = 'Debian' ]; then \
	    install -m 0755 script/debian_initscript /etc/init.d/badge_daemon ; \
	    sed -i s:'^DAEMON="badge_daemon"':'DAEMON="$(prefix)/sbin/badge_daemon"': /etc/init.d/badge_daemon ; \
	fi
	
	chmod +x script/db_update.sh
.PHONY: install
	
conf:
	mkdir -p $(confdir)
	install -m 0640 conf/hid_read.conf $(confdir)
	install -m 0640 conf/badge_daemon.conf $(confdir)
.PHONY: conf

webinstall: install
	mkdir -p $(wwwdir)/
	cp -rf CitofonoWeb $(wwwdir)/
	
	sed -i s:'/etc/badge_daemon':'$(confdir)': $(wwwdir)/CitofonoWeb/config.inc.php
	sed -i s:'/var/lib/citofonoweb/citofonoweb.db':'$(dbfile)': $(wwwdir)/CitofonoWeb/phpliteadmin/phpliteadmin.php
.PHONY: webinstall

db-update: install
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
