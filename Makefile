CC:=gcc
#CFLAGS:=-O2 -pipe -Wall -D_FORTIFY_SOURCE=2 -fstack-protector
CFLAGS:=-g -pipe -Wall -pedantic

prefix:=/usr/local
confdir:=/etc/badge_daemon

BACKEND:=sqlite
#Choose: hid_read serial_read door_open badge_daemon badge_logger badge_uploader
PROGRAMS:=hid_read serial_read door_open badge_daemon badge_logger badge_uploader
#Door tools - choose: gpio piface
DOOR_TOOLS:=gpio piface
#Logger tools - choose: buzzer lcdscreen
LOGGER_TOOLS:=buzzer lcdscreen
#Install examples - choose: text_feedback lcd_feedback remote_save_ssh
EXAMPLES:=

BOARD:="RASPBERRY"

GCCVERSION = $(shell $(CC) --version | grep ^gcc | sed -e 's/^.* //g' -e 's/\.[0-9]$$//g')
ifneq ($(filter arm%,$(shell uname -m)),)
    ifeq "$(GCCVERSION)" "4.6"
	override CFLAGS+= -fno-stack-protector
    endif
endif
ifeq "$(GCCVERSION)" "4.8"
    override CFLAGS+= -march=native
endif
ifeq "$(GCCVERSION)" "4.9"
    override CFLAGS+= -march=native
endif


ifeq ("$(BACKEND)","mysql")
    override LIBS+=-DMYSQL_B `mysql_config --cflags --libs`
else
    override LIBS+=-DSQLITE_B -lsqlite3
endif

ifneq ("$(wildcard /usr/include/json-c)","")
    override LIBS+=-ljson-c
endif
ifneq ("$(wildcard /usr/local/include/json-c)","")
    override LIBS+=-ljson-c
endif
ifneq ("$(wildcard /usr/include/json)","")
    override LIBS+=-Dljson -ljson
endif
ifneq ("$(wildcard /usr/local/include/json)","")
    override LIBS+=-Dljson -ljson
endif

ifeq "$(BOARD)" "RASPBERRY"
    override BOARD=1
endif
ifeq "$(BOARD)" "BEAGLEBONE"
    override BOARD=1
endif

OPTIONS:= -D_BOARD=$(BOARD)


all: $(PROGRAMS) $(DOOR_TOOLS) $(LOGGER_TOOLS)
.PHONY: all

door_open.o: door_open.c
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -std=gnu99 -DCONFPATH='"$(confdir)"' $< -c

door_open: door_open.o
	$(CC) $(CFLAGS) $(LIBS) $(OPTIONS) -std=gnu99 -DCONFPATH='"$(confdir)"' $< -o $@

badge_daemon.o: badge_daemon.c
	$(CC) $(CFLAGS) $(OPTIONS) -DCONFPATH='"$(confdir)"' $< -c

badge_daemon: badge_daemon.o
	$(CC) $(CFLAGS) $(OPTIONS) -lpthread -DCONFPATH='"$(confdir)"' $< -o $@

badge_logger: badge_logger.c badge_logger_common.o f_lock.o
	$(CC) $(CFLAGS) $(OPTIONS) $^ -o $@

badge_uploader: badge_uploader.c badge_logger_common.o f_lock.o
	$(CC) $(CFLAGS) $(OPTIONS) $^ -o $@

badge_logger_common.o: badge_logger_common.c
	$(CC) $(CFLAGS) $(OPTIONS) $< -o $@ -c

f_lock.o: f_lock.c
	$(CC) $(CFLAGS) $(OPTIONS) $< -o $@ -c

gpio.o: gpio.c
	$(CC) $(CFLAGS) $(OPTIONS) $< -o $@ -c

buzzer: buzzer.c gpio.o
	$(CC) $(CFLAGS) $(OPTIONS) -std=gnu99 $^ -o $@

lcdscreen: lcdscreen.c gpio.o f_lock.o
	$(CC) $(CFLAGS) $(OPTIONS) -std=gnu99 $^ -o $@

gpio:
.PHONY: gpio

remote_save_ssh:
.PHONY: remote_save_ssh

text_feedback:
.PHONY: remote_save_ssh

lcd_feedback:
.PHONY: remote_save_ssh

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


install: $(PROGRAMS) $(DOOR_TOOLS) $(LOGGER_TOOLS)
	getent passwd badge_daemon >/dev/null || useradd -r badge_daemon
	gpasswd -a badge_daemon spi || ( groupadd -r spi && gpasswd -a badge_daemon spi )
	gpasswd -a badge_daemon gpio || ( groupadd -r gpio && gpasswd -a badge_daemon gpio )
	gpasswd -a badge_daemon dialout || echo "Serial access already granted"
	install -m 644 resources/99-gpio-permissions.rules /etc/udev/rules.d/
	gpasswd -a badge_daemon input || ( groupadd -r input && gpasswd -a badge_daemon input )
	install -m 644 resources/99-input-permissions.rules /etc/udev/rules.d/
	udevadm control --reload-rules
	udevadm trigger

	mkdir -p $(prefix)/sbin
	install -m 0755 -t $(prefix)/sbin $(PROGRAMS)

	if [ -n "`echo $(DOOR_TOOLS) | grep gpio`" ]; then \
	    install -m 0755 -t $(prefix)/sbin resources/gpio.sh ; \
	fi

	if [ -n "`echo $(DOOR_TOOLS) | grep piface`" ]; then \
	    make -C piface_tool/ prefix=$(prefix) install ; \
	fi

	if [ -n "`echo $(EXAMPLES) | grep remote_save_ssh`" ]; then \
	    install -m 0755 -t $(prefix)/sbin examples/badge_logger/remote_save_ssh.sh ; \
	    if [ ! -e $(confdir)/.ssh/id_rsa ]; then \
		mkdir -p $(confdir)/.ssh/ ; \
		ssh-keygen -q -N "" -f $(confdir)/.ssh/id_rsa; \
	    fi ; \
	fi

	if [ -n "`echo $(EXAMPLES) | grep text_feedback`" ]; then \
	    install -m 0755 -t $(prefix)/sbin examples/badge_logger/text_feedback.sh ; \
	fi

	if [ -n "`echo $(EXAMPLES) | grep lcd_feedback`" ]; then \
	    install -m 0755 -t $(prefix)/sbin examples/badge_logger/lcd_feedback.sh ; \
	fi

	if [ -n "`echo $(LOGGER_TOOLS) | grep buzzer`" ]; then \
	    install -m 4755 -t $(prefix)/sbin buzzer ; \
	fi

	if [ -n "`echo $(LOGGER_TOOLS) | grep lcdscreen`" ]; then \
	    install -m 0755 -t $(prefix)/sbin lcdscreen ; \
	fi

	if [ -n "`echo $(PROGRAMS) | grep badge_logger`" ]; then \
	    mkdir -p /var/lib/badge_daemon/ ; \
	    chown -R badge_daemon:badge_daemon /var/lib/badge_daemon ; \
	    chmod -R 755 /var/lib/badge_daemon ; \
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

	if [ -z "`echo $(OPTIONS) | grep '(NO_LOGFILE|SYSTEMD_ONLY)'`" ]; then \
	    install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon ; \
	    mkdir -p /var/log/badge_daemon/ ; \
	    chown -R badge_daemon:badge_daemon /var/log/badge_daemon ; \
	    chmod -R 755 /var/log/badge_daemon ; \
	fi

	if which systemctl >/dev/null 2>&1 ; then \
	    install -m 0644 resources/badge_daemon.service /etc/systemd/system/badge_daemon.service ; \
	    install -m 0644 resources/badge_daemon@.service /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'^ExecStart=badge_daemon':'ExecStart=$(prefix)/sbin/badge_daemon': /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'-f conf/badge_daemon.conf':'-f $(confdir)/badge_daemon.conf': /etc/systemd/system/badge_daemon.service ; \
	    sed -i s:'-f conf/':'-f $(confdir)/': '/etc/systemd/system/badge_daemon@.service' ; \
	    systemctl enable badge_daemon ; \
	elif [ "`lsb_release -is`" = 'Debian' ]; then \
	    install -m 0755 resources/debian_initscript /etc/init.d/badge_daemon ; \
	    sed -i s:'^DAEMON="badge_daemon"':'DAEMON="$(prefix)/sbin/badge_daemon"': /etc/init.d/badge_daemon ; \
	    sed -i s:'^CONFDIR="conf"':'CONFDIR="$(confdir)"': /etc/init.d/badge_daemon ; \
	    insserv badge_daemon ; \
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
	rm -f $(prefix)/sbin/buzzer
	rm -f $(prefix)/sbin/lcdscreen

	rm -f $(prefix)/sbin/remote_save_ssh.sh
	rm -f $(prefix)/sbin/text_feedback.sh
	rm -f $(prefix)/sbin/lcd_feedback.sh
	#rm -f $(prefix)/sbin/piface_tool
	#rm -f $(prefix)/sbin/gpio.sh

	rm -f /etc/init.d/badge_daemon
	rm -f /etc/systemd/system/badge_daemon.service
	rm -f /etc/logrotate.d/badge_daemon
.PHONY: uninstall

clean:
	rm -f $(PROGRAMS) $(LOGGER_TOOLS) $(DOOR_TOOLS)
	rm -f *.o
.PHONY: clean
