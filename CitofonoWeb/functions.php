<?php
class tools{
	/**
	 * Lancia un comando e restituisce retval|stdout|all
	 * @param unknown_type $x
	 * @param unknown_type $return
	 */
	static function exec($x,$return='all',$html=true){
		if ($return=='retval'){
			exec($x,$b,$retval);
			return $retval;
		}
		else if ($return=='stdout'){
			$rx=shell_exec($x);
			if ($html)
				return str_replace("\n","<br/>",htmlentities($rx));
			return $rx;
		}
		else{
			$rx=shell_exec($x.' 2>&1');
			if ($html)
				return str_replace("\n","<br/>",htmlentities($rx));
			return $rx;
		}
	}

	static function key_fingerprint($ascii=false){
		if ($ascii){
			return str_replace(' ','&nbsp;',self::exec('ssh-keygen -lf /etc/ssh/ssh_host_ecdsa_key.pub -v'));
		}
		else{
			$key=self::exec('ssh-keygen -lf /etc/ssh/ssh_host_ecdsa_key.pub');
			if (preg_match('/\s([a-z0-9]{2}\:)+([a-z0-9]{2})\s/', $key, $match))
				return trim($match[0]);
			else
				return false;
		}
	}
    
    static function confkey($key,$file){
        $f=file_get_contents(config::confdir.$file);
        if (preg_match("/^$key (.*)$/", $f, $match)){
            return $match[1];
        }
        else{
            return false;
        }
    }
}