CitofonoWeb
===================

CitofonoWeb is a low-cost system to control doors with some hardware authentication module (MSR, RFID reader, keyboard, etc...) developed for Raspberry Pi.
It should work with any pc with some GPIOs and at least an USB port with some tuning.
There is an optional simple web interface (with phpSqliteAdmin included) to manage most important setting like time/date, network configuration and acls (which badges/codes can open the door and when).

How It Works
===============
It monitors a HID device: if an allowed sequence is received, it triggers a relay.

Dependencies
===============
[A step-by-step guide will be posted on the wiki]
Before you start dowloading, make sure you have all needed hardware & software.
- Hardware: a Raspberry Pi, a HID device, the board with relay (PiFace is a good choice)
- Software: libjson >= 0.11, libsqlite3, a working toolchain, piface-1.0 (optional - only if you want to use the PiFace)

The web interface needs a web server running as root, and should be protected with (at least) basic auth.
I'm quite happy with lighttpd, but you are free to use what you like. Note that apache2 complains if you configure it to run as root (and usually refuses to start).

Install dependencies on Debian:
- libjson-dev
- libsqlite3-dev
- build-essential

Web interface dependencies (with Lighttpd):
- apt-get install lighttpd php5-cgi php5-cli php5-sqlite git-core rsync lsb-release usbutils binutils at psmisc diffutils ntpdate sqlite3

Building from source
======================
You should choose how you want to open your door with the "MACHINE" parameter when you launch "make".
If you build your own relay board connected to GPIO, you can use:
- the generic sysfs interface -> make MACHINE=raspberry_sysfs
- the "gpio" command from https://projects.drogon.net/raspberry-pi/wiringpi/download-and-install/ -> make MACHINE=raspberry_gpio

If you have a PiFace:
- the piface-1.0 library -> make MACHINE=raspberry_piface

If you only want to test:
- the placeholder library (only prints debug messages to stderr) -> make MACHINE=placeholder (or "make" without parameters)

Installation
===============
When "make" has terminated, you can install with "make install".
If you need the web interface, run "make webinstall" after.
If you are very lazy, you can configure lighttpd with "make lighttpd-config".

Configuration
===============
You can find the configuration files in /etc/badge_daemon, the documentation is inside them.

- badge_daemon.conf: main daemon settings.
    They are pretty much self-explanatory.
- hid_read.conf: badge reader related settings.
    You need to change the device path (look in /dev/input/by-id/ for your device).
    DO NOT CHANGE "mode" and "outmode", badge_daemon won't like that.
    If you have a cheap and buggy device, you can tune "timeout" and "max_retry" parameters.


Usage
=======
- configure badge_daemon to be run at startup (on debian: "insserv -d badge_daemon")
- Test the web interface.

On the web interface, login as user "root" (default password: "admin"):
- MAINTENANCE -> Service management: choose the HID device to monitor.
- MAINTENANCE -> Service management: start the service.
- HOME -> Schedulation: set the acls.

If you need other infos contact me and i'll add more documentation.
