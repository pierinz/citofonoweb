<?php
include_once 'framework.php';
require_once ("functions.php");

$cms=new cms("Shutdown/Restart",'');

if ($cms->user=='root'){
?>
<h1>Restart device</h1>
<form action="controller.php" method="post">
	<p><input type="hidden" name="act" value="restart"/></p>
	<p><input type="submit" value="Restart" /></p>
</form>
	<p>The device will come up again in about 50 seconds.</p>
	<br/><br/><br/>
<form action="controller.php" method="post">
	<p><input type="hidden" name="act" value="shutdown"/></p>
	<p><input type="submit" value="Shutdown" /></p>
</form>
<p>Be sure to have physical access to this device before halting.</p>
<?php
}
else
{
?>
<h1>Restart device</h1>
<form action="controller.php" method="post">
	<p><input type="hidden" name="act" value="restart"/></p>
	<p><input type="submit" value="Restart" /></p>
</form>
<p>The device will come up again in about 50 seconds.</p>
<?php
}
?>
