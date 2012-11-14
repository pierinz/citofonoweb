<?php
include_once 'framework.php';
require_once ("functions.php");

$cms=new cms("Service management");
$cms->ACL('admin');
?>
<h1>Service Status</h1>
<p>The service is <?php 
if (file_exists('/var/run/badge_daemon.pid') && posix_kill(file_get_contents('/var/run/badge_daemon.pid'),0))
	echo 'RUNNING';
else
	echo 'STOPPED';
?>
.</p>

<?php
if (utils::inGroup($cms->user)=='admin'){
?>
<h2>Service management</h2>
<form action="controller.php" method="post">
	<p><input type="hidden" name="act" value="service"/></p>
	<p>Action: 
		<select name="signal">
			<option>Start</option>
			<option>Stop</option>
			<option>Restart</option>
			<option>Kill</option>
		</select>
	</p>
	<p><input type="submit" value="Send signal" /></p>
</form>

<h2>Input device selection</h2>
<p>You need to restart the daemon after doing this.</p>
<form action="controller.php" method="post">
	<p>Input device: <select name="dev">
<?php
	if (file_exists('/dev/input/by-id')){
		$devlist=tools::exec('ls /dev/input/by-id/ -1');
		$devices=explode("\n", $devlist);
		echo '<option value="'.config::device.'">'.basename(config::device).'</option>';
		foreach ($devices as $d){
			$d=str_replace('<br/>', '', $d);
			echo '<option value="/dev/input/by-id/'.$d.'">'.$d.'</option>';
		}
	}
	else{
		echo '<option disabled="disabled" value="null">Device not connected</option>';
	}
?>
	</select></p>
	<p><input type="hidden" name="act" value="inputedit"/><input type="submit" value="Change" /></p>
</form>
<?php
}
