<?php
require_once ("framework.php");
require_once ("functions.php");

$update=file_get_contents('/var/log/badge_daemon.log');
$cms=new cms('Badge daemon log','');
?>
<h1>Badge daemon log</h1>
<p class="log">
<?php echo nl2br(htmlentities($update),1);?>
</p>
<p style="width: 650px; text-align: right;"><a href="dlog.php?l=badge_daemon">Download log</a></p>
<p style="width: 650px; text-align: right;"><a href="oldlogs.php">Older logs</a></p>