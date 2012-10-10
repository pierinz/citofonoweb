<?php
require_once ("framework.php");
require_once ("functions.php");

$head='<style type="text/css">
div.net p{
	width: 350px;
}
div.net p span{
	float: right;
}
</style>';

$cms=new cms("LAN Status",$head);
$cms->ACL('admin');


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

$mac=trim(`ifconfig $iface | egrep -o '([0-9a-z]{2}:){5}[0-9a-z]{2}'`);

$route=`route -n | grep UG | grep $iface`;
$gw=preg_replace('/0\.0\.0\.0 +([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}).*/','${1}',$route);

foreach (file('/etc/resolv.conf') as $row){
	if (preg_match('/^nameserver (.*)/',$row,$m))
		$dns[]=$m[1];
	if (preg_match('/^domain (.*)/',$row,$m))
		$domain=$m[1];	
	if (preg_match('/^search (.*)/',$row,$m))
		$search=$m[1];
}
?>

<h1>LAN Configuration</h1>
<div class="net">
<p>IP address: <span>&nbsp;<?php echo $ip; ?></span></p>
<p>Netmask: <span>&nbsp;<?php echo $nmask; ?></span></p>
<p>Broadcast address: <span>&nbsp;<?php echo $bcast; ?></span></p>
<p>Gateway: <span>&nbsp;<?php echo $gw; ?></span></p>
<p>MAC address: <span>&nbsp;<?php echo $mac; ?></span></p>
<p><input type="button" onclick="document.location.href='network-lan-edit.php?edit=iface'" value="Change configuration" /></p>
<br/><br/>
<p>DNS 1: <span>&nbsp;<?php echo @$dns[0] ?></span></p>
<p>DNS 2: <span>&nbsp;<?php echo @$dns[1] ?></span></p>
<p>DNS 3: <span>&nbsp;<?php echo @$dns[2] ?></span></p>
<p>Domain: <span>&nbsp;<?php echo @$domain ?></span></p>
<p>Search: <span>&nbsp;<?php echo @$search ?></span></p>
<p><input type="button" onclick="document.location.href='network-lan-edit.php?edit=resolv'" value="Change configuration" /></p>
</div>
