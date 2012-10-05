<?php
#Configurazioni predefinite
@date_default_timezone_set(str_replace("\n",'',`cat /etc/timezone`));
setlocale(LC_TIME, 'it_IT.utf8');
setlocale(LC_NUMERIC, 'en_US');

#Compressione pagine
ini_set("zlib.output_compression",1);


#Inibisci gli effetti di magic_quotes_gpc se settato
if (get_magic_quotes_gpc()){
	function stripslashes_gpc(&$value){
		$value = stripslashes($value);
	}
	array_walk_recursive($_GET, 'stripslashes_gpc');
	array_walk_recursive($_POST, 'stripslashes_gpc');
	array_walk_recursive($_COOKIE, 'stripslashes_gpc');
	array_walk_recursive($_REQUEST, 'stripslashes_gpc');
}

#Se presente, carica la configurazione
if (file_exists(dirname(__FILE__).'/config.inc.php'))
	include_once(dirname(__FILE__).'/config.inc.php');

class template{
	private $head='';
	private $menubar='1';
	private $templatename='default';
	
	/**
	 * Restituisce la cartella del template in uso
	 */
	function themepath(){
		return utils::webroot()."template/$this->templatename/";
	}
	
	/**
	 * Stampa il prologo e la <head> del documento
	 * @param unknown_type $title
	 */
	function genHead($title){
		include_once("template/$this->templatename/document.php");
		echo '<head>';
		include_once("template/$this->templatename/head.php");
		echo "<title>$title</title>";
		echo $this->head;
		echo '</head>';
	}
	
	/**
	 * Stampa il menù come definito nel template
	 */
	function genMenu(){		
		include_once("template/$this->templatename/menu.php");
	}
	
	/**
	 * Stampa la struttura della pagina così come definito nel template
	 */
	function openBody(){
		echo "<body>";
		include_once("template/$this->templatename/header.php");
		echo '<div id="page">';
		if ($this->menubar)
			$this->genMenu();
		echo '<div id="content">';
		include_once("template/$this->templatename/body.php");
	}
	
	/**
	 * Chiude la struttura della pagina
	 */
	function closeBody(){
		echo '</div></br></div></body></html>';
	}
	
	/*
	 * Costruttore (apre automaticamente la struttura della pagina)
	 * parametri: il titolo della pagina, eventuali cose da aggiungere nella <head> e se mettere la barra dei menù
	 */
	function __construct($title='',$head='',$menubar='1'){
		clearstatcache();
		if (file_exists(dirname(__FILE__).'/config.inc.php')){
			$this->templatename=config::template;
		}
		$this->head=$head;
		$this->menubar=$menubar;
		$this->genHead($title);
		$this->openBody();
	}
	
	/**
	 * Distruttore (chiude automaticamente la struttura della pagina)
	 */
	function __destruct(){
		$this->closeBody();
	}
	
}

class utils{
	/**
	 * Restituisce la corretta document root con tanto di trailing slash se non presente
	 */
	static function documentroot(){
		if (!preg_match('#/$#',$_SERVER['DOCUMENT_ROOT']))
			return $_SERVER['DOCUMENT_ROOT'].'/';
		return $_SERVER['DOCUMENT_ROOT'];
	}
	
	/**
	 * Restituisce la cartella in cui è stato installato il CMS
	 */
	static function webroot(){
		if (strpos(utils::documentroot(),dirname(__FILE__))){
			$webroot=str_replace(utils::documentroot(),'',dirname(__FILE__));
			if (!preg_match('#/$#',$webroot))
				$webroot.='/';
			if (!preg_match('#^/#',$webroot))
				$webroot='/'.$webroot;
			return $webroot;
		}
		else{
			$dirname=dirname($_SERVER['SCRIPT_FILENAME']);
			if (preg_match('#(administration|install)#',$dirname))
				$dirname=dirname($dirname);
			if (preg_match('#template#',$dirname))
				$dirname=dirname(dirname($dirname));

			$webroot=str_replace(utils::documentroot(),'',$dirname);
			if (!preg_match('#/$#',$webroot))
				$webroot.='/';
			if (!preg_match('#^/#',$webroot))
				$webroot='/'.$webroot;
			return $webroot;
		}
	}
	
	/**
	 * Restituisce la cartella in cui è installato il cms
	 */
	static function pwd(){
		return str_replace('//','/',utils::documentroot().utils::webroot());
	}
	
	/**
	 * Genera una password casuale lunga $len
	 * @param unknown_type $len
	 */
	static function gen_password($len = 8)
	{
	    // function calculates 32-digit hexadecimal md5 hash
	    // of some random data
	    return substr(md5(rand().rand()), 0, $len);
	}

	static function inGroup($user){
		include(self::documentroot().self::webroot().'groups.php');
		$group=json_decode($json,true);
		return @$group[$user];
	}
}

class cms{
	private $link;
	private $template;
	private $loginbox='';
	private $lite=false;
	
	#Alla costruzione lo riempie con il valore di $_SERVER['PHP_AUTH_USER']
	public $user='';
	
	/*
	 * Costruttore predefinito
	 */
	function __construct($title='',$head='',$menubar='1'){
		if ($title=='' && $head=='' && $menubar=='1'){
			$this->liteConstruct();
		}
		else{
			#Se non è presente la configurazione, abortisci
			clearstatcache();
			if (!file_exists(dirname(__FILE__).'/config.inc.php'))
				die('Configurazione mancante. Installa il CMS e riprova.');

			$this->template=new template($title,$head,$menubar);
		}

		if (!isset($_SERVER['PHP_AUTH_USER'])){
			die('Configurazione server errata: questa cartella deve essere protetta da password');
		}
		$this->user=$_SERVER['PHP_AUTH_USER'];
	}
	
	/**
	 * Costruttore alternativo: non inizializza il template
	 */
	function liteConstruct(){
		#Tieni presente che non c'è il template
		$this->lite=true;
		#Se non è presente la configurazione, abortisci
		clearstatcache();
		if (!file_exists(dirname(__FILE__).'/config.inc.php'))
			die('Configurazione mancante. Installa il CMS e riprova.');
		
		$this->pf=config::prefix;
	}
	
	
	function ACL($group){
		if (utils::inGroup($_SERVER['PHP_AUTH_USER'])!=$group){
			if ($this->lite)
				die('Non hai i privilegi necessari per visualizzare questa pagina.');
			else
				die('<h1>Non hai i privilegi necessari per visualizzare questa pagina.</h2>');
		}
	}
	
	function updateUser($field,$value){
		if (!preg_match('/^(group|password)$/',$field))
			return false;
		#TODO
	}

	function resources(){
		return $this->template->themepath().'resources/';
	}
	function thumbs(){
		return utils::webroot().'thumbnails/';
	}
	function logout(){
		#TODO
	}
	
	function __destruct(){
		#Nulla per ora
	}
}