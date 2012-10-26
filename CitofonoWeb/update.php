<?php
require_once ("framework.php");
require_once ("functions.php");

$cms=new cms("Update");
$cms->ACL('admin');
?>

<h1>Updates</h1>
<p>These functions take a long time to run. Sit down and wait.</p>
<h2>System</h2>
<p>Check for updates: <button onclick="document.location.href='controller.php?act=pkgupdate'">Update list</button>
<button onclick="document.location.href='controller.php?act=pkgupgrade'">Upgrade packages</button></p>
<h2>Firmware</h2>
<p><button onclick="document.location.href='controller.php?act=fwscript'">Update firmware-update-script</button></p>
<p><button onclick="document.location.href='controller.php?act=fwupdate'">Launch firmware-update-script</button></p>