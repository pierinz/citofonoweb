<?php
include_once 'framework.php';

if ($_REQUEST['l']=='syslog')
	$log=file_get_contents('/var/log/syslog');
elseif ($_REQUEST['l']=='badge_daemon')
	$log=file_get_contents('/var/log/badge_daemon.log');
else
	$log='';

header('Content-type: text/plain');
header('Content-Disposition: attachment; filename="'.$_REQUEST['l'].'.log"');
header("Cache-Control: must-revalidate");
header("Expires: ".gmdate("D, d M Y H:i:s",time()+(60 * 60 * 24 * 365 * 2)) . " GMT");
echo $log;
?>