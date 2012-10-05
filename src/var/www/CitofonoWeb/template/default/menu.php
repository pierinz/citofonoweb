<div id="menubar">
	<?php
	if (isset ($_SERVER['PHP_AUTH_USER'])){
		if (utils::inGroup($_SERVER['PHP_AUTH_USER'])=='admin'){
			$menu=array('HOME'=>array(
					'Status'=>'index.php',
					'Extended status'=>'info.php',
					'Schedulation'=>'sched.php',
					'I/O'=>'io.php'
				),
				'NETWORK'=>array(
					'Lan'=>'network-lan.php'
				),
				'LOGS'=>array(
					'Syslog'=>'syslog.php',
					'Badge daemon'=>'badgelog.php'
				),
				'MAINTENANCE'=>array(
					'Remote Management'=>'management.php',
					'Service Management'=>'service.php',
					'Password'=>'passwd.php',
					'Date/Time'=>'datetime.php',
					'Update board'=>'update.php',
					'Restart'=>'shutdown.php'
				)
				#Non implementato	'Diagnostics'=>'diagnostics.php'
			);
		}
		else{
			$menu=array(
				'HOME'=>array(
					'Status'=>'index.php',
					'Extended status'=>'info.php',
					'Schedulation'=>'sched.php'
				),
				'LOGS'=>array(
					'Syslog'=>'syslog.php'
				),
				'MAINTENANCE'=>array(
					'Service status'=>'service.php',
					'Password'=>'passwd.php',
					'Date/Time'=>'datetime.php'
				)
			);
		}
	
		foreach($menu as $key=>$value){
			echo '<p>'.$key.'</p>
			<ul>';
				foreach ($value as $el=>$vl)
					echo '<li><a href="'.utils::webroot().$vl.'">'.$el.'</a></li>';
			echo '</ul>';
		}
	}
	?>
</div>