<?php
include_once 'framework.php';

if ($_REQUEST['l']=='syslog' && (utils::inGroup($_SERVER['PHP_AUTH_USER'])=='admin'))
	$log='/var/log/syslog';
elseif ($_REQUEST['l']=='badge_daemon.log')
	$log='/var/log/badge_daemon.log';
elseif (preg_match ('/badge_daemon\.log\.[0-9]+(\.gz)?/',$_REQUEST['l']))
	$log='/var/log/'.$_REQUEST['l'];
else
	$log='/dev/null';

header('Content-type: text/plain');
header('Content-Disposition: attachment; filename="'.$_REQUEST['l'].'.log"');
header("Cache-Control: must-revalidate");
header("Expires: ".gmdate("D, d M Y H:i:s",time()+(60 * 60 * 24 * 365 * 2)) . " GMT");

#Print log to stdout
readgzfile($log);
?>