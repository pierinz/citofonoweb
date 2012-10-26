CitofonoWeb
===================

CitofonoWeb is a low-cost system to control doors with some hardware authentication module (MSR, RFID reader, keyboard, etc...) developed for Raspberry Pi.
Theorically it should work with any pc with some GPIOs and at least an USB port with some tuning.
This application comes with a simple web interface to manage most important setting like time/date, network configuration and acls (which badges/codes can open the door and when).

How It Works
===============
It uses a daemon that monitors a HID device: if an allowed sequence is received, it triggers a relay.

Installation
===============

[A step-by-step guide will be posted on the wiki]
Before you start dowloading, make sure you have all needed hardware & software.
- Hardware: a Raspberry Pi, a HID device, the board with relay (more info on the website)
- Software: you need a web server running as root, and the web interface should be protected with (at least) basic auth.
I'm quite happy with lighttpd, but you are free to use what you like. Note that apache2 complains if you configure it to run as root (and usually refuses to start).

Install dependencies (on Debian with Lighttpd):
- apt-get install lighttpd php5-cgi php5-cli php5-sqlite git-core rsync lsb-release usbutils binutils at psmisc diffutils ntpdate sqlite3
- you need also the "gpio" command from https://projects.drogon.net/raspberry-pi/wiringpi/download-and-install/

Now, just cd in the source folder and launch "make && make install".

Usage
=======

- Configure lighttpd (see lighttpd.conf.example in examples folder)
- configure badge_daemon to be run at startup (on debian: "insserv -d badge_daemon")
- Test the web interface.

On the web interface, login as user "root":
- MAINTENANCE -> Service management: choose the HID device to monitor.
- MAINTENANCE -> Service management: start the service.
- HOME -> Schedulation: set the acls.

If you need other infos contact me and i'll add more documentation.