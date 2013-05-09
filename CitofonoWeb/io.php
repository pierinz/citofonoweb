<?php
require_once ("framework.php");
require_once ("functions.php");

$cms=new cms("Input/Output");
$cms->ACL('admin');
?>
<h1>Input/Output</h1>

<h2>Test GPIO</h2>
<table border="1" cellspacing="1" cellpadding="5">
	<thead>
		<tr>
			<th>Pin number</th><th>Action</th>
		</tr>
	</thead>
	<tbody>
<?php

$pins=array(4,17,18,21,22,23,24,25);
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

$statusled=tools::confkey('statusled', 'badge_daemon.conf');
$doorpin=tools::confkey('doorpin', 'badge_daemon.conf');
$alarmpin=tools::confkey('alarmpin', 'badge_daemon.conf');
?>
	</tbody>
</table>

<h2>Bind GPIO</h2>
<form action="controller.php" method="post">
	<table border="1" cellspacing="1" cellpadding="5">
		<thead>
			<tr>
				<th></th><th>Pin number</th><th>Change to</th>
			</tr>
		</thead>
		<tbody>
			<tr>
				<td>Status LED</td>
				<td><?php echo $statusled; ?></td>
				<td>
					<select name="statusled">
						<option><?php echo $statusled; ?></option>
						<option>4</option>
						<option>17</option>
						<option>18</option>
						<option>21</option>
						<option>22</option>
						<option>23</option>
						<option>24</option>
						<option>25</option>
					</select>
				</td>
			</tr>
			<tr>
				<td>Door open</td>
				<td><?php echo $doorpin; ?></td>
				<td>
					<select name="doorpin">
						<option><?php echo $doorpin; ?></option>
						<option>4</option>
						<option>17</option>
						<option>18</option>
						<option>21</option>
						<option>22</option>
						<option>23</option>
						<option>24</option>
						<option>25</option>
					</select>
				</td>
			</tr>
			<tr>
				<td>Buzzer</td>
				<td><?php echo $alarmpin; ?></td>
				<td>
					<select name="buzzer">
						<option><?php echo $alarmpin; ?></option>
						<option>4</option>
						<option>17</option>
						<option>18</option>
						<option>21</option>
						<option>22</option>
						<option>23</option>
						<option>24</option>
						<option>25</option>
					</select>
				</td>
			</tr>
		</tbody>
	</table>
	<p>
		<input type="hidden" name="act" value="chbinding"/>
		<input type="Submit" value="Change bindings"/>
	</p>
</form>