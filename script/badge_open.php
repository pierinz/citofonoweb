#!/usr/bin/php
<?php
$wwwdir='/var/www';
require_once($wwwdir.'/CitofonoWeb/config.inc.php');
require_once($wwwdir.'/CitofonoWeb/functions.php');

#Default config
@date_default_timezone_set(str_replace("\n",'',`cat /etc/timezone`));
setlocale(LC_TIME, 'it_IT.utf8');
setlocale(LC_NUMERIC, 'en_US');

$preamble=config::preamble;
$following=config::following;
$terminate=array("\n");
$device=config::device;

//Parse options
$options = getopt("vd::");

if (isset($options['d']))
	$device=$options['d'];

// signal handler function
function clean_close(){
	global $logger, $listener, $stdin, $stderr, $link;
	if (isset($options['v'])){
		file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Terminating...\n");
	}
	fwrite($logger,date('Y-m-d H:i:s')." - Daemon stopped.\n");
	fclose($logger);
	
	#Terminate child process
	fclose($stdin);
	fclose($stderr);
	$s = proc_get_status($listener);
	$ppid=$s['pid'];
	$pids = preg_split('/\s+/', `ps -o pid --no-heading --ppid $ppid`);
    foreach($pids as $pid) {
        if(is_numeric($pid)) {
            if (isset($options['v'])){
            	file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Killing $pid\n");
            }
            posix_kill($pid, 9); //9 is the SIGKILL signal
        }
    }
	posix_kill($ppid, SIGTERM);
	proc_close($listener);
	#Close db connection
	$link=null;
	exit();
}

function rotate(){
	global $logger;
	if (isset($options['v'])){
		file_put_contents('php://stderr', date('Y-m-d H:i:s')." - SIGHUP received! Rotating...");
	}
	#Close logfile
	fclose($logger);
	
	#Wait logrotate
	while (file_exists(config::logname)){
		usleep(100000);
	}
	#Create a new logfile & open it
	$logger=fopen(config::logname,'w');
	fwrite($logger,date('Y-m-d H:i:s')." - Rotated logfile.\n");
}

function parseline($line){
	global $link, $logger, $preamble, $following, $options;
	if (isset($options['v']))
		echo "line = $line \n";
	fwrite($logger,date('Y-m-d H:i:s')." - line = $line \n");
	$line=preg_replace("/^$preamble/",'',$line);
	$line=preg_replace("/$following$/m",'',$line);

	$line=trim($line);
	
	if ($line==''){
		fwrite($logger,date('Y-m-d H:i:s')." -------> Line was empty, discarding. \n");
		return false;
	}
	
	$query="select description,allowed,sched from acl where badge_code='$line'";
	try{
		$rx=$link->query($query);
	}
	catch(PDOException $e){
		fwrite($logger,date('Y-m-d H:i:s')." - ".$e->getMessage());
		clean_close();
	}
	fwrite($logger,date('Y-m-d H:i:s')." -------> badge code: ".$line."\n");
	fwrite($logger,date('Y-m-d H:i:s')." -------> ");
	$i=0;
	foreach ($rx as $row){
		if ($row['allowed']){
			$json=json_decode($row['sched'],1);
			if (date('His')>$json[date('N')]['start'] && date('His')<$json[date('N')]['end']){
				if (isset($options['v']))
					echo "Door open\n";
				tools::door_open();
				fwrite($logger,"badge '".$row['description']."' => door open \n");
				usleep(500000);
			}
			else{
				if (isset($options['v']))
					echo "Unauthorized access\n";
				tools::door_deny();
				fwrite($logger,"badge '".$row['description']."' => unauthorized access \n");
			}
		}
		else{
			if (isset($options['v']))
				echo "Disabled badge\n";
			tools::door_deny();
			fwrite($logger,"badge '".$row['description']."' => unauthorized access \n");
		}
		$i++;
	}
	if ($i==0){
		if (isset($options['v']))
			echo "Unknown badge\n";
		tools::door_unknown();
		fwrite($logger,"badge $line => unknown \n");
	}
}

function parsecode($code){
	$keycodes=array("", "<esc>", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
		"-", "=", "<backspace>",
		"<tab>", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[",
		"]", "\n", "<control>", "a", "s", "d", "f", "g",
		"h", "j", "k", "l", ";", "'", "", "<shift>",
		"\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
		"/", "<shift>", "", "<alt>", " ", "<capslock>", "<f1>",
		"<f2>", "<f3>", "<f4>", "<f5>", "<f6>", "<f7>", "<f8>", "<f9>",
		"<f10>", "<numlock>", "<scrolllock>", "", "", "", "", "", "", "",
		"", "", "", "\\", "f11", "f12", "", "", "", "", "", "",
		"", "", "<control>", "", "<sysrq>","\n","\n");
	if (isset($keycodes[$code]))
		return $keycodes[$code];
	else
		return "{?$code}";
}


$logger = fopen(config::logname,'a');
fwrite($logger,date('Y-m-d H:i:s')." - Daemon started \n");

try{
	$link = new PDO('sqlite:'.config::localdb);
}
catch(PDOException $e){
	fwrite($logger,date('Y-m-d H:i:s')." - DB missing or corrupted (remember to create the database!): ".$e->getMessage()." \n");
	fwrite($logger,date('Y-m-d H:i:s')." - Crashing. \n");
	fclose($logger);
	die("DB missing or corrupted: ".$e->getMessage());
}

#Write pid file
file_put_contents('/var/run/badge_daemon.pid',getmypid());
if (isset($options['v'])){
	file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Process started: ".getmypid()."\n");
}

$descriptorspec = array(
   #0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
   1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
   2 => array("pipe", "w") // stderr is a pipe to write to
);

$lastlaunch=time(1);
$failures=0;

do{
	#If the process lasts more than 50s, reset the failure counter
	if ($failures>0 && time(1)>($lastlaunch+50)){
		fwrite($logger,date('Y-m-d H:i:s')." - Last run was succesful - crash counter set to 0. \n");
		$failures=0;
	}
	
	fwrite($logger,date('Y-m-d H:i:s')." - Listening on $device... \n");
	$listener = proc_open("badge_listener $device", $descriptorspec, $pipes);
	$stdin=&$pipes[1];
	$stderr=&$pipes[2];
	stream_set_blocking($stdin,0);
	stream_set_blocking($stderr,0);
	
	// setup signal handlers
	pcntl_signal(SIGTERM, "clean_close");
	pcntl_signal(SIGINT, "clean_close");
	pcntl_signal(SIGHUP,  "rotate");
	
	pcntl_signal_dispatch();

	$lastline='';
	$ltime=microtime(1);
	$buffer='';
	while (!feof($stdin)){
		$line = trim(fgets($stdin)); // reads one char from STDIN
		pcntl_signal_dispatch();
		if (in_array(parsecode($line),$terminate)){
			#Debounce input
			if (microtime(1)>($ltime+(config::debouncetime))){
				parseline($buffer);
				$ltime=microtime(1);
				$lastline=$buffer;
			}
			else{
				if($buffer != $lastline){
					parseline($buffer);
					$ltime=microtime(1);
					$lastline=$buffer;
				}
			}
			$buffer='';
		}
		elseif($line==''){
			usleep(500000);
		}
		else{
			$buffer.=parsecode($line);
		}
		
		if (strlen($buffer)>40){
			fwrite($logger,date('Y-m-d H:i:s')." - Dirty buffer: $buffer.\n");
			$buffer='';
			fwrite($logger,date('Y-m-d H:i:s')." - Buffer forcefully cleared. The next swipe might be ignored.\n");
		}
		#usleep(500);
	}
	
	#If we get there the child process has crashed
	fwrite($logger,date('Y-m-d H:i:s')." - Child process dead -> trace:\n");
	while (!feof($stderr)){
		$err=trim(fgets($stderr));
		if ($err!='')
			fwrite($logger,date('Y-m-d H:i:s')." - ".$err."\n");
	}
	$failures++;
	#Loop if keepalive is enabled
}while (config::keepalive && $failures<config::maxcrashes);

if ($failures>=config::maxcrashes){
	fwrite($logger,date('Y-m-d H:i:s')." - The child process crashed too many times. Service stopped.\n");
}

#close & exit
clean_close();
?>
