<?php
require_once ("framework.php");
require_once ("functions.php");

$head=<<<EOF
<style type="text/css">
div#container{
	width: 95%;
	margin-left: auto;
	margin-right: auto;
	height: 140px;
	overflow: auto;
}
div.cat{
	float: left;
	margin-top: 10px;
	margin-left: 5px;
	margin-right: 5px;
	text-align: center;
	height: 130px;
	width: 160px;
	background-color: transparent;
}
div.cat a{
	text-decoration: none;
}

table{
	min-width: 60%;
}
tr.red{
	background-color: red;
}
</style>
EOF;

$cms=new cms("Schedulation",$head);
?>

<h1>Schedulation</h1>
<table border="1" cellspacing="1" cellpadding="5">
	<thead>
		<tr>
			<th>Badge</th><th>Description</th><th>Enabled</th><th>Mon</th><th>Tue</th><th>Wed</th><th>Thu</th><th>Fri</th><th>Sat</th><th>Sun</th>
		</tr>
	</thead>
	<tbody>
<?php 

try{
    $localdb=tools::confkey('dbfile', 'badge_daemon.conf');
	$link = new PDO('sqlite:'.$localdb);
	$query="select badge_code, description, allowed, sched from acl order by badge_code asc";
	$rx=$link->query($query);
	
	if (count($rx)==0)
		echo '<tr><td colspan="10">No data</td></tr>';
	else{
		foreach($rx as $row){
			$class='';
			if ($row['allowed']==0)
				$class='red';
			echo '<tr class="'.$class.'">
			<td>'.$row['badge_code'].'</td>
			<td>'.$row['description'].'</td>
			<td>'.$row['allowed'].'</td>';
			$json=json_decode(stripslashes($row['sched']),1);
			foreach ($json as $day){
				echo '<td>'.preg_replace('/^([0-9]{2})([0-9]{2})[0-9]{2}$/', '${1}:${2}', $day['start']).' - '.preg_replace('/^([0-9]{2})([0-9]{2})[0-9]{2}$/', '${1}:${2}', $day['end']).'</td>';
			}
			echo '</tr>';
		}
	}
}
catch(PDOException $e){
	if ($e->getCode()==14)
		echo '<tr><td colspan="9">Errore: devi ancora creare il database</td></tr>';
	else
		echo '<tr><td colspan="9">Errore: '.$e->getMessage().'</td></tr>';
}
?>
	</tbody>
</table>
<h2>Actions</h2>
<p><button onclick="if (confirm('Current data will be erased. Are you sure you want to do this?')){document.location.href='dbexample.php';}">Create DB and populate with example data</button></p>
<form action="phpliteadmin/index.php" method="post">
<p>
	<input type="hidden" name="password" value="admin"/>
	<input type="hidden" name="remember" value="yes"/>
	<input type="hidden" name="login" value="Log In"/>
	<input type="hidden" name="proc_login" value="true"/>
	<input type="submit" value="DB editor"/>
</p>
</form>
