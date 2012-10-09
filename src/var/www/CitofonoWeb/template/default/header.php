<div id="header">
<a href="<?php echo utils::webroot(); ?>"><img id="logo" src="<?php echo $this->themepath(); ?>resources/logo.png" alt="Logo" /></a>
<span id="devname"><?php echo config::name; ?></span>
<?php
if (@$_SERVER['PHP_AUTH_USER']){
?>
	<form id="login" action="<?php echo utils::webroot(); ?>logout.php" method="post">
		<p>Welcome, <?php echo $_SERVER['PHP_AUTH_USER'] ?></p>
		<p><button type="submit" style="float: right;">Logout</button></p>
	</form>
<?php 
}
else{
?>
	<form id="login">
		<p>Server configuration error</p>
		<p>You are required to log in to view this page.</p>
	</form>
<?php
}
?>
</div>