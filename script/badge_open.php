#!/usr/bin/php
<?php
$wwwdir='/var/www';
require_once($wwwdir.'/CitofonoWeb/config.inc.php');
require_once($wwwdir.'/CitofonoWeb/functions.php');

#Configurazioni predefinite
@date_default_timezone_set(str_replace("\n",'',`cat /etc/timezone`));
setlocale(LC_TIME, 'it_IT.utf8');
setlocale(LC_NUMERIC, 'en_US');

//load keymap
$keymap=implode("\n",readgzfile(config::keymapfile);
$preamble=config::preamble;
$following=config::following;
$terminate=array('Return','KP_Enter');
$device=config::device;

$verbose=false;
//Parse options
$options = getopt("vd::k::");

if (isset($options['d']))
	$device=$options['d'];
if (isset($options['k']))
	$keymap=file_get_contents($options['k']);
if (isset($options['k']) && $options['k'])
	$verbose=true;

// signal handler function
function clean_close(){
	global $logger, $listener, $stdin, $link;
	if (@$options['v']){
		file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Terminating...\n");
	}
	fwrite($logger,date('Y-m-d H:i:s')." - Daemon stopped.\n");
	fclose($logger);
	#Termina il processo che legge dal lettore badge
	fclose($stdin);
	$s = proc_get_status($listener);
	$ppid=$s['pid'];
	$pids = preg_split('/\s+/', `ps -o pid --no-heading --ppid $ppid`);
    foreach($pids as $pid) {
        if(is_numeric($pid)) {
            if (@$options['v']){
            	file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Killing $pid\n");
            }
            posix_kill($pid, 9); //9 is the SIGKILL signal
        }
    }
	posix_kill($ppid, SIGTERM);
	proc_close($listener);
	#Chiudi la connessione al db
	$link=null;
	exit();
}

function rotate(){
	global $logger;
	if (@$options['v']){
		file_put_contents('php://stderr', date('Y-m-d H:i:s')." - SIGHUP received! Rotating...");
	}
	fclose($logger);
	#Wait logrotate finish its work
	while (file_exists(config::logname)){
		usleep(100000);
	}
	$logger=fopen(config::logname,'w');
	fwrite($logger,date('Y-m-d H:i:s')." - Rotated logfile.\n");
}

function parseline($line){
	global $link, $logger, $preamble, $following, $options;
	if (@$options['v'])
		echo "line = $line \n";
	fwrite($logger,date('Y-m-d H:i:s')." - line = $line \n");
	$line=preg_replace("/^$preamble/",'',$line);
	$line=preg_replace("/$following$/m",'',$line);
	#$line=$line*1;
	$line=trim($line);
	$query="select allowed,sched from acl where badge_code='$line'";
	try{
		$rx=$link->query($query);
	}
	catch(PDOException $e){
		fwrite($logger,date('Y-m-d H:i:s')." - ".$e->getMessage());
		clean_close();
	}
	fwrite($logger,date('Y-m-d H:i:s')." -------> ");
	$i=0;
	foreach ($rx as $row){
		if ($row['allowed']){
			$json=json_decode($row['sched'],1);
			if (date('His')>$json[date('N')]['start'] && date('His')<$json[date('N')]['end']){
				if (@$options['v'])
					echo "Door open\n";
				tools::door_open();
				fwrite($logger,"badge $line => door open \n");
				usleep(500000);
			}
			else{
				if (@$options['v'])
					echo "Unauthorized access\n";
				tools::door_deny();
				fwrite($logger,"badge $line => unauthorized access \n");
			}
		}
		else{
			if (@$options['v'])
				echo "Unauthorized access\n";
			tools::door_deny();
			fwrite($logger,"badge $line => unauthorized access \n");
		}
		$i++;
	}
	if ($i==0){
		if (@$options['v'])
			echo "Unknown badge\n";
		tools::door_unknown();
		fwrite($logger,"badge $line => unknown \n");
	}
}

function parsecode($code){
	global $keymap;
	if ($code==11)
		return '0';
	if ($code>1 && $code<11)
		return $code-1;

	preg_match_all("/keycode(\s)+ $code = (.*)\s/", $keymap, $match);
	if (isset($match[2][0]))
		return $match[2][0];
	else
		return 'Unknown';
}


$logger = fopen(config::logname,'a');
fwrite($logger,date('Y-m-d H:i:s')." - Daemon started \n");

try{
	$link = new PDO('sqlite:'.config::localdb);
}
catch(PDOException $e){
	fwrite($logger,date('Y-m-d H:i:s')." - DB missing or corrupted (remember to sync db before start): ".$e->getMessage()." \n");
	fwrite($logger,date('Y-m-d H:i:s')." - Crashing. \n");
	fclose($logger);
	die("DB missing or corrupted: ".$e->getMessage());
}

#Write pid file
file_put_contents('/var/run/badge_daemon.pid',getmypid());
if (@$options['v']){
	file_put_contents('php://stderr', date('Y-m-d H:i:s')." - Process started: ".getmypid()."\n");
}

$descriptorspec = array(
   #0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
   1 => array("pipe", "w")  // stdout is a pipe that the child will write to
   #2 => array("pipe", "w") // stderr is a file to write to
);
$listener = proc_open("badge_listener $device", $descriptorspec, $pipes);
$stdin=&$pipes[1];
stream_set_blocking($stdin,0);

// setup signal handlers
pcntl_signal(SIGTERM, "clean_close");
pcntl_signal(SIGINT, "clean_close");
pcntl_signal(SIGHUP,  "rotate");

pcntl_signal_dispatch();

$lastline='';
$ltime=microtime(1);
$buffer='';
while (!feof($stdin)){
	$line = trim(fgets($stdin)); // reads one line from STDIN
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
	elseif($line=="\n" || $line==''){
		usleep(500000);	
	}
	else{
		$buffer.=parsecode($line);
		#echo $line.' '.parsecode($line)."\n";
	}
	
	if (strlen($buffer)>40){
		fwrite($logger,date('Y-m-d H:i:s')." - Dirty buffer: $buffer.\n");
		$buffer='';
		fwrite($logger,date('Y-m-d H:i:s')." - Buffer forcefully cleared. The next swipe might be ignored.\n");
	}
	#usleep(500);
}

clean_close();
?>
