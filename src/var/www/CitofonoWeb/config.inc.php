<?php
class config{
	#Template predefinito
	const template='default';
	
	const iface='eth0';

	const ntpservers='192.168.0.2,192.168.0.1,0.debian.pool.ntp.org';

	const device='/dev/input/by-id/usb-ID_TECH_ID_TECH_OMNI_TM3_USB-HID_KB_CONVERTER-event-kbd';
	#garbage before badge code
	const preamble='ò11121200';
	#garbage after badge code
	const following='0000000000\-*Shift\-*';
	#Debounce time (s): ignore same input for specified time
	const debouncetime=2;
	
	#Database locale sqlite
	const localdb='/var/lib/citofonoweb/citofonoweb.db';
	
	#Utente sola lettura db locale
	const readeruser='citofonoweb';
	const readerpwd='citofonoweb';
	
	#Percorso log
	const logname='/var/log/badge_daemon.log';
	
	const doorpin='24';
	const doortime=1;
	
	const redled='23';
	const redledtime=3;
	
	const buttonpin='21';
	
	const buzzer='18';
}
?>