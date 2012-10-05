<?php
require_once ("framework.php");
require_once ("functions.php");

$head='<style type="text/css">

form p input{
	width: 100px;
	float: right;
}
</style>';

$cms=new cms("Change password",$head);
?>
<h1>Change password</h1>
<form action="controller.php" method="post">
<p>New password: <input name="pw" type="password" value="" />
<input type="hidden" name="act" value="changepw"/></p>
<p>Repeat new password: <input name="rpw" type="password" value="" /></p>
<p><input style="width:105px" type="submit" value="Change" /></p>   
</form>

<?php
if ($cms->user=='root'){
	$users=explode("\n",`cat /etc/lighttpd/lighttpd-plain.user | cut -d':' -f1`);
	
	$opts='';
	foreach ($users as $i){
		if (trim($i))
			$opts.='<option>'.trim($i).'</option>';
	}
?>

<h1>Change user password</h1>
	<form action="controller.php" method="post">
		<p>User: <select name="user"><?php echo $opts; ?></select></p>
		<p>New password: <input name="pw" type="password" value="" /><input type="hidden" name="act" value="changepw"/></p>
		<p>Repeat new password: <input name="rpw" type="password" value="" /></p>
		<p><input type="submit" value="Change" /></p>
	</form>

<?php
}
?>