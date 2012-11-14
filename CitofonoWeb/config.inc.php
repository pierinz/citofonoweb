<?php
class config{
	#Default template
	const template='default';
	
	#Network interface
	const iface='eth0';
	#Comma-separated list of ntp servers
	const ntpservers='0.debian.pool.ntp.org,1.debian.pool.ntp.org,2.debian.pool.ntp.org';

	#HID device to monitor
	const device='/dev/input/by-id/usb-ID_TECH_ID_TECH_OMNI_TM3_USB-HID_KB_CONVERTER-event-kbd';
	#garbage before badge code (regexp)
	const preamble='ò11121[0-9]00';
	#garbage after badge code (regexp)
	const following='0000000000\/*Shift\/*';
	#Debounce time (s): ignore same input for specified time
	const debouncetime=2;
	
	#Description of this device
	const name='Citofonoweb #1';
	
	#Local db (sqlite3)
	const localdb='/var/lib/citofonoweb/citofonoweb.db';
	#Log path
	const logname='/var/log/badge_daemon.log';
	#Keymap file path
	const keymapfile='/usr/share/keymaps/i386/qwerty/us.kmap.gz';
	
	#GPIO number connected to the door
	const doorpin='24';
	#Pulse duration
	const doortime=1;
	
	#GPIO number connected to the red led
	const redled='23';
	#Pulse duration
	const redledtime=3;
	
	#GPIO number connected to the status led
	const statusled='17';
	
	#GPIO number connected to the input button
	const buttonpin='21';
	
	#GPIO number connected to the buzzer
	const buzzer='18';
}
?>