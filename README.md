CitofonoWeb
====================

CitofonoWeb is a low-cost system to control doors with some hardware authentication
module (MSR, RFID reader, keyboard, etc...) developed for the Raspberry Pi.
It has some modules to perform various actions and/or support more devices.
It should work with any pc with some GPIOs and at least an USB port with some tuning.

How It Works
====================
The basic workflow is simple: it monitors a HID device, and if an allowed sequence
is received, it opens the door (you should configure how).
You can change the configuration to read from serial device instead of HID,
to switch from GPIO output to PiFace output and many more.

Database
====================
The default CitofonoWeb module needs a database to store users and keys data.
You can choose at compile time the database with the BACKEND directive:
"mysql" or "sqlite".
- Sqlite is the best choice for a single unit with rare changes: it is a local 
	database only, very	lightweight, but editing isn't straightforward.
- MySql is the best choice for a large number of units: you can connect them to
	a remote database and manage the configuration from a single place.

Dependencies
====================
Before you start dowloading, make sure you have all needed hardware & software.
- Hardware: a Raspberry Pi (or any board you like), a HID device, a board with
	relay (PiFace is a good choice)
- Software: libjson >= 0.11, sqlite3 or mysql, a working toolchain

On Debian you need those packages:
- libjson-dev
- libsqlite3-dev | libmysqlclient-dev (choose one)
- build-essential

Building from source
====================
You should choose which tools you need to open your door at compile time.
By default you will get both gpio and piface tools, but you can exclude one
with the DOOR_TOOLS directive.
If you don't exclude the piface tools, they will be fetched and compiled from
their repository.
You should also choose a BACKEND as described before.

When you are ready, launch:
- make BACKEND=(yourbackend) DOOR_TOOLS=(toolsyouchoose)

If you are confortable with default values (BACKEND="sqlite" DOOR_TOOLS="gpio piface"),
just run "make".

Installation
====================
When "make" has terminated, you can install with "make install".

Configuration
====================
The default configuration file is /etc/badge_daemon/badge_daemon.conf, the documentation is inside it.
You can split the configuration for "door_open" or the other modules if you like it.

HID device tuning
====================
The "source" directive in the configuration file by default use the hid_read program.
You can tune various parameters, run "hid_read -h" to get an extended explanation.

Briefly, you have to change the device path (look in /dev/input/by-id/ for your device).
If you have a cheap and buggy device, you can tune "timeout" (-t) and 
"max_retry" (-r) parameters to get better results.


Usage
====================
- Edit configuration file
- Be sure to create folders with correct privileges (if needed)
- Launch "badge_daemon" to test if it is working
- Configure badge_daemon to be run at startup

You can find a SysV initscript and a systemd unit in the "resources" folder.
The initscript has the LSB header, so you can simply use "insserv" to enable it
in the right runlevels.
For convenience, "make install" will enable the daemon at startup for you.

If you need other infos contact me and i'll add more documentation.
