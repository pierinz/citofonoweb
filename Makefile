CC:=gcc
CFLAGS:=-g -pipe -Wall

prefix:=/usr/local
confdir:=/etc/badge_daemon
wwwdir:=/var/www
dbfile:=/var/lib/badge_daemon/citofonoweb.db

BACKEND:=sqlite
PROGRAMS:=hid_read serial_read door_open badge_daemon
DOOR_TOOLS:=gpio piface

ifeq ("$(BACKEND)","mysql")
    LIBS:=-DMYSQL_B `mysql_config --cflags --libs` -lpthread
else
    LIBS:=-DSQLITE_B -lsqlite3 -lpthread
endif

all: $(PROGRAMS) $(DOOR_TOOLS)
.PHONY: all

door_open.o: door_open.c
	if [ -e '/usr/include/json-c' ] || [ -e '/usr/local/include/json-c' ]; then \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)"' -ljson-c $< -c ; \
	else \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)"' -Djson -ljson $< -c ; \
	fi

door_open: door_open.o
	if [ -e '/usr/include/json-c' ] || [ -e '/usr/local/include/json-c' ]; then \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)"' -ljson-c $< -o $@ ; \
	else \
	    $(CC) $(CFLAGS) $(LIBS) -std=gnu99 -DCONFPATH='"$(confdir)"' -Djson -ljson $< -o $@ ; \
	fi

badge_daemon.o: badge_daemon.c
	$(CC) $(CFLAGS) $(LIBS) -DCONFPATH='"$(confdir)"' $< -c

badge_daemon: badge_daemon.o
	$(CC) $(CFLAGS) $(LIBS) -DCONFPATH='"$(confdir)"' $< -o $@

gpio:
.PHONY: gpio

piface:
	if [ ! -e ./piface_tool ]; then \
	    git clone https://github.com/pierinz/piface_tool.git ; \
	else \
	    cd piface_tool ; \
	    git pull ; \
	    cd .. ; \
	fi
	make -C piface_tool/ prefix=$(prefix)
	cd ..
.PHONY: piface

install: $(PROGRAMS)
	mkdir -p $(prefix)/sbin
	install -m 0755 -t $(prefix)/sbin $^
	
	if [ -n "`echo $(DOOR_TOOLS) | grep gpio`" ]; then \
	    install -m 0755 -t $(prefix)/sbin resources/gpio.sh ; \
	fi
	
	if [ -n "`echo $(DOOR_TOOLS) | grep piface`" ]; then \
	    make -C piface_tool/ prefix=$(prefix) install ; \
	fi
	
	mkdir -p $(confdir)
	if [ -e $(confdir)/badge_daemon.conf ]; then \
	    echo "badge_daemon.conf found - skipping" ; \
	    echo "Run 'make conf' to overwrite" ; \
	else \
	    install -m 0640 conf/badge_daemon.conf $(confdir) ; \
	    sed -i s:' ./':' $(prefix)/sbin/': $(confdir)/badge_daemon.conf ; \
	fi
	
	mkdir -p `dirname $(dbfile)`
	install -m 0644 resources/db.info `dirname $(dbfile)`
	
	install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon
	if [ "`lsb_release -is`" = 'Debian' ]; then \
	    install -m 0755 resources/debian_initscript /etc/init.d/badge_daemon ; \
	    sed -i s:'^DAEMON="badge_daemon"':'DAEMON="$(prefix)/sbin/badge_daemon"': /etc/init.d/badge_daemon ; \
	    sed -i s:'^CONFDIR="conf"':'CONFDIR="$(confdir)"': /etc/init.d/badge_daemon ; \
	fi
	if [ -e '/usr/bin/systemctl' ]; then \
	    install -m 0644 resources/badge_daemon.service /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'^ExecStart=badge_daemon':'ExecStart=$(prefix)/sbin/badge_daemon': /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'-f conf/badge_daemon.conf':'-f $(confdir)/badge_daemon.conf': /etc/systemd/system/badge_daemon.service ; \
	    systemctl enable badge_daemon ; \
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
	rm -f *.o
.PHONY: clean
