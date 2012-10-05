<?php
require_once ("framework.php");
require_once ("functions.php");

$cms=new cms("Input/Output");
?>
<h1>Input/Output</h1>
<table border="1" cellspacing="1" cellpadding="5">
	<thead>
		<tr>
			<th>Pin number</th><th>Action</th>
		</tr>
	</thead>
	<tbody>
<?php

$pins=array(18,23,24);
foreach ($pins as $pin){
?>
		<tr>
			<td><?php echo $pin; ?></td>
			<td>
				<button onclick="document.location.href='gpio.php?action=on&pin=<?php echo $pin; ?>'">On</button>
				<button onclick="document.location.href='gpio.php?action=off&pin=<?php echo $pin; ?>'">Off</button>
				<button onclick="document.location.href='gpio.php?action=pulse&pin=<?php echo $pin; ?>'">Pulse</button>
			</td>
		</tr>
<?php 
}
?>
	</tbody>
</table>