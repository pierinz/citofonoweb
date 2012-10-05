<?php
require_once ("framework.php");
require_once ("functions.php");

$head=<<<EOF
<script type="text/javascript">
function remkey(key){
	document.location.href="controller.php?act=remkey&key="+key;
}
</script>
EOF;

$cms=new cms("Remote Management",$head);
?>
<h1>Remote Management</h1>
<p>If you have a central administration panel, you should add it there to allow management of this device.</p>

<h2>Authorized keys</h2>
<table border="1" cellspacing="1" cellpadding="5">
	<thead>
		<tr>
			<th>Key</th><th>Host</th><th></th>
		</tr>
	</thead>
	<tbody>
<?php
foreach (file('/root/.ssh/authorized_keys') as $k){
	if($k!="\n"){
		echo '<tr>';
		$key=explode(' ', $k);
		echo '<td style="font-family: monospace">'.$key[0].' '.substr($key[1],0,25).'...</td>';
		echo '<td>'.$key[2].'</td>';
		echo '<td><button onclick="remkey(\''.base64_encode($k).'\');">Revoke authorization</button></td>';
		echo '</tr>';
	}
}
?>
	</tbody>
</table>

<h2>Add new key</h2>
<form action="controller.php" method="post">
<p>Public key: <textarea name="key" rows="2" cols="400" style="width: 300px;"></textarea><input type="hidden" name="act" value="addkey" /></p>
<p><input type="submit" value="Add key"/></p>
</form>

<h2>Public host key</h2>
<p>This device is identified by its public host key: <span style="font-family: monospace;"><?php echo tools::key_fingerprint(true); ?></span></p>
<p>USE THE BUTTON BELOW ONLY IF YOU ENCOUNTER PROBLEMS (i.e. when you clone sd card - duplicate keys). You will need to update your local .ssh/known_hosts file.</p>
<p><button onclick="document.location.href='controller.php?act=regenerate'">Regenerate host keys</button></p>