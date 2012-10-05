<div id="header">
<a href="<?php echo utils::webroot(); ?>"><img id="logo" src="<?php echo $this->themepath(); ?>resources/logo.png" alt="Logo" /></a>
<?php
if (@$_SERVER['PHP_AUTH_USER']){
?>
	<form id="login" action="<?php echo utils::webroot(); ?>logout.php" method="post">
		<p>Benvenuto, <?php echo $_SERVER['PHP_AUTH_USER'] ?></p>
		<p><button type="submit" style="float: right;">Logout</button></p>
	</form>
<?php 
}
else{
?>
	<form id="login">
		<p>Errore di configurazione server</p>
		<p>È necessario autenticarsi per vedere questa pagina</p>
	</form>
<?php
}
?>
</div>