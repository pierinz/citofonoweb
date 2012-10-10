<?php
require_once ("framework.php");
require_once ("functions.php");

$head='<style type="text/css">
form#net p{
	width: 250px;
}
form#net p input{
	width: 100px;
	float: right;
}
</style>';

$cms=new cms("LAN Settings",$head);
$cms->ACL('admin');
?>
<h1>Edit network configuration</h1>


<?php
if ($_GET['edit']=='iface'){
	$iface=config::iface;
	$ifconfig=`ifconfig $iface | grep inet`;
	preg_match('/[^0-9]([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})[^0-9].*[^0-9]([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})[^0-9].*[^0-9]([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})([^0-9]|$)/',$ifconfig,$matches);

	$ip=@$matches[1];
	if (preg_match('/^255\./', $matches[2])){
		$nmask=$matches[2];
		$bcast=@$matches[3];
	}
	else{
		$nmask=$matches[3];
		$bcast=@$matches[2];
	}
	
	$route=`route -n | grep UG | grep $iface`;
	$gw=trim(preg_replace('/0\.0\.0\.0 +([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}).*/','${1}',$route));

	echo '<form id="net" method="post" action="controller.php">
	<p>IP address: <input name="ip" value="'.$ip.'" /></p>
	<p>Netmask: <input name="nmask" value="'.$nmask.'" /></p>
	<p>Gateway: <input name="gw" value="'.$gw.'" /></p>
	<p>The network interface will be restarted.</p>
	<p><input type="hidden" name="act" value="net-if" />
	<input type="submit" value="Apply" /></p>
	</form>';
}
elseif ($_GET['edit']=='resolv'){
	foreach (file('/etc/resolv.conf') as $row){
		if (preg_match('/^nameserver (.*)/',$row,$m))
			$dns[]=$m[1];
		if (preg_match('/^domain (.*)/',$row,$m))
			$domain=$m[1];	
		if (preg_match('/^search (.*)/',$row,$m))
			$search=$m[1];
	}
	echo '<form method="post" action="controller.php">
	<p>DNS 1: <input name="dns1" value="'.@$dns[0].'" /></p>
	<p>DNS 2: <input name="dns2" value="'.@$dns[1].'" /></p>
	<p>DNS 3: <input name="dns3" value="'.@$dns[2].'" /></p>
	<p>Domain: <input name="domain" value="'.@$domain.'" /></p>
	<p>Search: <input name="search" value="'.@$search.'" /></p>
	<p><input type="hidden" name="act" value="net-dns" />
	<input type="submit" value="Apply" /></p>
	</form>';
	
}
else
	die('Injection detected. Aborted.');
?>