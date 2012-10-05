<?php
require_once ("framework.php");
require_once ("functions.php");

$head=<<<EOF
<style type="text/css">
div#container{
	width: 95%;
	margin-left: auto;
	margin-right: auto;
	height: 140px;
	overflow: auto;
}
div.cat{
	float: left;
	margin-top: 10px;
	margin-left: 5px;
	margin-right: 5px;
	text-align: center;
	height: 130px;
	width: 160px;
	background-color: transparent;
}
div.cat a{
	text-decoration: none;
}

table{
	min-width: 60%;
}
</style>
EOF;

$cms=new cms("Home page",$head);

$df=`LC_ALL=C df -h`;
$df=str_replace('File system','Filesystem',$df);
$df=str_replace('Mounted on','Mountpoint',$df);
$df=preg_replace('/\ +/',' ',$df);

$table='<table border="1" cellspacing="1" cellpadding="5">';
$row=explode("\n",$df);
foreach ($row as $i){
	if (trim($i)){
		$table.='<tr>';
		$field=explode(' ',$i);
		foreach ($field as $f)
				$table.='<td>'.$f.'</td>';
		$table.='</tr>';
	}
}
$table.='</table>';

$free=`LC_ALL=C free -h`;
$free=str_replace('-/+ buffers/cache:', '-/+&nbsp;buffers/cache:', $free);
$free=preg_replace('/\ +/',' ',$free);
$tablem='<table border="1" cellspacing="1" cellpadding="5">';
$row=explode("\n",$free);
foreach ($row as $i){
	if (trim($i)){
		$tablem.='<tr>';
		$field=explode(' ',$i);
		$k=0;
		foreach ($field as $f){
				$tablem.='<td>'.$f.'</td>';
				$k++;
		}
		for ($z=$k;$z<7;$z++)
			$tablem.='<td></td>';
		
		$tablem.='</tr>';
	}
}
$tablem.='</table>';

?>

<h1>Device status</h1>

<h2>Info</h2>
<p><?php echo `uname -a`; ?></p>
<p><?php echo `lsb_release -a`; ?></p>
<p><?php echo `uptime`; ?></p>

<h2>Service status</h2>
<p>The service is <?php 
if (file_exists('/var/run/badge_daemon.pid') && posix_kill(file_get_contents('/var/run/badge_daemon.pid'),0))
	echo 'RUNNING';
else
	echo 'STOPPED';
?>
.</p>

<h2>Memory usage</h2>
<?php echo $tablem; ?>

<h2>Disk usage</h2>
<?php echo $table; ?>
<br/>

<h2>USB Peripherals</h2>
<?php echo tools::exec("lsusb -t"); ?>

<h2>Running processes</h2>
<?php echo str_replace(' ', '&nbsp;', tools::exec("pstree")); ?>