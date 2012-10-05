<?php
include_once 'framework.php';

if ($_POST['l']=='syslog')
	$log=file_get_contents('/var/log/syslog');
elseif ($_POST['l']=='badge_daemon')
	$log=file_get_contents('/var/log/badge_daemon.log');
else
	$log='';

header('Content-type: text/plain');
header('Content-Disposition: attachment; filename="'.$_GET['l'].'.log"');
header("Cache-Control: must-revalidate");
header("Expires: ".gmdate("D, d M Y H:i:s",time()+(60 * 60 * 24 * 365 * 2)) . " GMT");
echo $log;
?>