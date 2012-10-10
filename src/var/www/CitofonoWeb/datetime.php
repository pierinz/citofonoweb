<?php
require_once ("framework.php");
require_once ("functions.php");

$head='<style type="text/css">
form.indented p{
	width: 350px;
}
form.indented p input, form p select{
	width: 120px;
	float: right;
}
</style>';

$timezone=glob('/usr/share/zoneinfo/*/*');

$cms=new cms("Date/Time",$head);
?>

<h1>Date/Time</h1>
<p>System time: <?php echo date('d/m/Y H:i:s');?></p>
<p>Timezone: <?php echo date_default_timezone_get();?></p>

<h2>NTP</h2>

<form class="indented" action="controller.php" method="post">
	<p><input type="hidden" name="act" value="ntpget"/>
	<input type="submit" value="Get date/time from NTP Server" /></p>
</form>

<?php
if (utils::inGroup($cms->user)=='admin'){
?>
<p>NTP Servers: </p>
<form class="indented" action="controller.php" method="post">
<?php
	$ntp=explode(',',config::ntpservers);
	$i=1;
	foreach ($ntp as $n){
		echo "<p>NTP Server #$i: ".'<input name="ntp'.$i.'" value="'.$n.'"/>'."</p>";
		$i++;
		if ($i>3)
			break;
	}
?>
	<p><input type="hidden" name="act" value="ntpset"/><input type="submit" value="Change" /><p>
</form>
<p><br/></p>

<h2>Edit time</h2>
<form class="indented" action="controller.php" method="post">
	<p>Date: <input name="date" value="<?php echo date('d/m/Y');?>" />
	<input type="hidden" name="act" value="dateset"/></p>
	<p>Time: <input name="time" value="<?php echo date('H:i:s');?>" /></p>
	<p>Timezone:
		<select name="timezone">
<?php
	echo '<option>'.date_default_timezone_get().'</option>';
	foreach ($timezone as $t){
		echo '<option>'.str_replace('/usr/share/zoneinfo/','',$t).'</option>';
	}
?>
		</select>
	</p>
	<p><input type="submit" value="Change" /></p>
</form>
<?php
}
?>
