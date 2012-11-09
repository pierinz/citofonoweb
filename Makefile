CC:=gcc
CFLAGS:=-Wall -O2

prefix:=/usr/local
resources:=/usr/local/share
wwwdir:=/var/www

badge_listener: badge_listener.c
	$(CC) $(CFLAGS) $< -o $@

install: badge_listener
	mkdir -p $(prefix)/sbin

	install -m 0755 $< $(prefix)/sbin/
	install -m 0755 script/badge_daemon.sh $(prefix)/sbin/
	sed -i s:'^webpath=.*':webpath=\"$(wwwdir)\"\: $(prefix)/sbin/badge_daemon.sh
	
	install -m 0755 script/badge_open.php $(prefix)/sbin/
	sed -i s:'^$$wwwdir=.*':\$$wwwdir=\"$(wwwdir)\"\;: $(prefix)/sbin/badge_open.php
	
	install -m 0755 script/debian_initscript /etc/init.d/badge_daemon
	sed -i s:'^PATH':'PATH=$(prefix)/sbin\:': /etc/init.d/badge_daemon

	install -m 0644 conf/badge_daemon.logrotate /etc/logrotate.d/badge_daemon
	mkdir -p $(prefix)/usr/share/badge_daemon/
	install -m 0644 resources/it_simplified.map $(resources)/badge_daemon/
	mkdir -p $(prefix)/var/lib/citofonoweb/
	install -m 0644 resources/db.info $(prefix)/var/lib/citofonoweb/readme

	mkdir -p $(wwwdir)/
	cp -rf CitofonoWeb $(wwwdir)/
	sed -i s:'/var/lib/citofonoweb/citofonoweb.db':'$(prefix)/var/lib/citofonoweb/citofonoweb.db': $(wwwdir)/CitofonoWeb/config.inc.php
	sed -i s:'/usr/local/share/badge_daemon/it_simplified.map':'$(resources)/badge_daemon/it_simplified.map': $(wwwdir)/CitofonoWeb/config.inc.php
    
.PHONY: install

uninstall:
	rm -f $(prefix)/usr/sbin/badge_listener
	rm -f $(prefix)/usr/sbin/badge_daemon.sh
	rm -f $(prefix)/usr/sbin/badge_open.php

	rm -f /etc/init.d/badge_daemon
	rm -f /etc/logrotate.d/badge_daemon
	rm -rf $(resources)/badge_daemon
	rm -rf $(prefix)/var/lib/citofonoweb
	rm -rf $(wwwdir)/CitofonoWeb

.PHONY: uninstall

clean:
	rm -f badge_listener

.PHONY: clean
