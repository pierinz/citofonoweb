<?php
require_once ("framework.php");
require_once ("functions.php");

$update=file_get_contents('/var/log/syslog');
$cms=new cms('System log','');
$cms->ACL('admin');
?>
<h1>Syslog</h1>
<p class="log">
<?php echo nl2br(htmlentities($update),1);?>
</p>
<p style="width: 650px; text-align: right;"><a href="dlog.php?l=syslog">Download log</a></p>
