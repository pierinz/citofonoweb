<?php
require_once ("framework.php");
require_once ("functions.php");

$cms=new cms("Example data insertion",'');
?>
<h1>Example data insertion</h1>
<p>
<?php
$insert="insert into `acl` (`badge_code`, `description`, `allowed`, `sched`) values ";

touch(config::localdb);
try{
	$local = new PDO('sqlite:'.config::localdb);
	$query="CREATE TABLE IF NOT EXISTS acl (badge_code varchar(50) PRIMARY KEY, description varchar(50), allowed tinyint, sched text)";
	$local->exec($query);
}
catch(PDOException $e){
		die("DB error: ".$e->getMessage());
}

$insert.=<<<EOF
('83', '', 1, '{"1":{"start":"090000","end":"180000"},"2":{"start":"090000","end":"180000"},"3":{"start":"090000","end":"180000"},"4":{"start":"090000","end":"180000"},"5":{"start":"090000","end":"180000"},"6":{"start":"000000","end":"000000"},"7":{"start":"000000","end":"000000"}}'),
('107', '', 0, '{"1":{"start":"090000","end":"180000"},"2":{"start":"090000","end":"180000"},"3":{"start":"090000","end":"180000"},"4":{"start":"090000","end":"180000"},"5":{"start":"090000","end":"180000"},"6":{"start":"000000","end":"000000"},"7":{"start":"000000","end":"000000"}}'),
('1000000022', '', 1, '{"1":{"start":"090000","end":"180000"},"2":{"start":"090000","end":"180000"},"3":{"start":"090000","end":"180000"},"4":{"start":"090000","end":"180000"},"5":{"start":"090000","end":"180000"},"6":{"start":"000000","end":"000000"},"7":{"start":"000000","end":"000000"}}'),
('1000000012', '', 1, '{"1":{"start":"000001","end":"235959"},"2":{"start":"000001","end":"235959"},"3":{"start":"000001","end":"235959"},"4":{"start":"000001","end":"235959"},"5":{"start":"000001","end":"235959"},"6":{"start":"000001","end":"235959"},"7":{"start":"000001","end":"235959"}}')
EOF;

try{
	$local->exec('delete from acl');
	$local->exec($insert);
	echo 'Example data inserted.';
}
catch(PDOException $e){
	die ("Error during insert: ".$e->getMessage());
}
?>
</p>
