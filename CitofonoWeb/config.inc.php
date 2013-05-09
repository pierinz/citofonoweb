<?php
class config{
	#Default template
	const template='default';
	
	#Network interface
	const iface='eth0';
	#Comma-separated list of ntp servers
	const ntpservers='0.debian.pool.ntp.org,1.debian.pool.ntp.org,2.debian.pool.ntp.org';
	
	#Description of this device
	const name='Citofonoweb #1';
    
    #Config directory
    const confdir='/etc/badge/daemon/';
}
?>