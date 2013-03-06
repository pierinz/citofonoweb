<?php
require_once ("framework.php");
require_once ("functions.php");

$cms=new cms('Older badge daemon logs','');
$list='<ul>';
foreach(glob("/var/log/badge_daemon.log.*") as $l){
    $list.='<li><a href="dlog.php?l='.basename($l).'">'.basename($l).'</li>';
}
$list.='</ul>';
?>
<h1>Older badge daemon logs</h1>
<?php echo $list; ?>