CC:=gcc
CFLAGS:=-g -pipe -Wall -D_FORTIFY_SOURCE=2 -fstack-protector

prefix:=/usr/local
confdir:=/etc/badge_daemon

BACKEND:=sqlite
PROGRAMS:=hid_read serial_read door_open badge_daemon
DOOR_TOOLS:=gpio piface
OPTIONS:=

ifeq ("$(BACKEND)","mysql")
    override LIBS+=-DMYSQL_B `mysql_config --cflags --libs` -lpthread
else
    override LIBS+=-DSQLITE_B -lsqlite3 -lpthread
endif

all: checklibs
.PHONY: all
	
checklibs:
	if [ -e '/usr/include/json-c' ] || [ -e '/usr/local/include/json-c' ]; then \
	   make LIBS+=" -ljson-c" $(PROGRAMS) $(DOOR_TOOLS) ; \
	else \
	   make LIBS+=" -Dljson -ljson" $(PROGRAMS) $(DOOR_TOOLS) ; \
	fi
.PHONY: checklibs

door_open.o: door_open.c
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -std=gnu99 -DCONFPATH='"$(confdir)"' $< -c ; \

door_open: door_open.o
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -std=gnu99 -DCONFPATH='"$(confdir)"' $< -o $@ ; \

badge_daemon.o: badge_daemon.c
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -DCONFPATH='"$(confdir)"' $< -c

badge_daemon: badge_daemon.o
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -DCONFPATH='"$(confdir)"' $< -o $@

badge_logger: badge_logger.c
	$(CC) $(CFLAGS) $(OPTIONS) `curl-config --libs` $< -o $@

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
	useradd -r badge_daemon || echo "User already present"
	gpasswd -a badge_daemon gpio || ( groupadd -r gpio && gpasswd -a badge_daemon gpio && install -m 644 resources/99-gpio-permissions.rules /etc/udev/rules.d/ )
	gpasswd -a badge_daemon input || install -m 644 resources/99-input-permissions.rules /etc/udev/rules.d/
	udevadm control --reload-rules

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
	    sed -i -e s:' ./':' $(prefix)/sbin/': $(confdir)/badge_daemon.conf -e s:' conf/':' $(confdir)/': $(confdir)/badge_daemon.conf ; \
	fi
	
	chown -R badge_daemon:badge_daemon $(confdir)
	chmod 755 $(confdir)
	chown -R badge_daemon:badge_daemon $(confdir)/badge_daemon.conf
	
	if [ -n "`echo $(OPTIONS) | grep '(NO_LOGFILE|SYSTEMD_ONLY)'`" ]; then \
	    install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon ; \
	    mkdir -p /var/log/badge_daemon/ ; \
	    chown -R badge_daemon:badge_daemon /var/log/badge_daemon ; \
	    chmod -R 755 /var/log/badge_daemon ; \
	fi
	
	if [ -e '/usr/bin/systemctl' ]; then \
	    install -m 0644 resources/badge_daemon.service /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'^ExecStart=badge_daemon':'ExecStart=$(prefix)/sbin/badge_daemon': /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'-f conf/badge_daemon.conf':'-f $(confdir)/badge_daemon.conf': /etc/systemd/system/badge_daemon.service ; \
	    systemctl enable badge_daemon ; \
	elif [ "`lsb_release -is`" = 'Debian' ]; then \
	    install -m 0755 resources/debian_initscript /etc/init.d/badge_daemon ; \
	    sed -i s:'^DAEMON="badge_daemon"':'DAEMON="$(prefix)/sbin/badge_daemon"': /etc/init.d/badge_daemon ; \
	    sed -i s:'^CONFDIR="conf"':'CONFDIR="$(confdir)"': /etc/init.d/badge_daemon ; \
	fi
.PHONY: install

conf:
	mkdir -p $(confdir)
	install -m 0640 conf/badge_daemon.conf $(confdir)
.PHONY: conf

uninstall:
	rm -f $(prefix)/sbin/badge_daemon
	rm -f $(prefix)/sbin/hid_read
	rm -f $(prefix)/sbin/serial_read
	rm -f $(prefix)/sbin/door_open
	rm -f $(prefix)/sbin/badge_logger
	#rm -f $(prefix)/sbin/piface_tool
	#rm -f $(prefix)/sbin/gpio.sh

	rm -f /etc/init.d/badge_daemon
	rm -f /etc/systemd/system/badge_daemon.service
	rm -f /etc/logrotate.d/badge_daemon
.PHONY: uninstall

clean:
	rm -f $(PROGRAMS)
	rm -f *.o
.PHONY: clean
