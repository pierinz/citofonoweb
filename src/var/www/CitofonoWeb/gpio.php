<?php
require_once ("framework.php");
require_once ("functions.php");

$head='<meta http-equiv="refresh" content="3; url='.$_SERVER['HTTP_REFERER'].'" />';

$cms=new cms("Input/Output",$head);
$cms->ACL('admin');
?>
<h1>Input/Output</h1>
<?php
if (in_array($_GET['pin'],array(4,17,18,21,22,23,24,25)))
	$pin=$_GET['pin'];
else
	die('Invalid pin number');

if ($_GET['action']=='on' || $_GET['action']=='pulse')
	$out=1;
elseif ($_GET['action']=='off')
	$out=0;
else
	die('Invalid action');

echo tools::exec("/usr/local/bin/gpio -g mode $pin out");
echo tools::exec("/usr/local/bin/gpio -g write $pin $out");

if ($_GET['action']=='pulse'){
	sleep(1);
	echo tools::exec("/usr/local/bin/gpio -g write $pin 0");
}
?>
<p>Command executed.</p>
