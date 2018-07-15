<?php
require_once "../httpi.inc";

$con = httpi_connect();

if($con==null) die("Could not connect to database\n");

try {
	$con->beginTransaction();
	$con->query("DROP TABLE accesslog");
	$con->query("DROP TABLE authlog");
	$con->query("DROP TABLE tokens");
	echo("Dropped tables... ");
	if(httpi_setup_database($con)) {
		echo("OK\r\n");	
		$con->commit();
	}
	else {
		echo("Error recreating tables\n");
		$con->rollback();
			
	}
}
catch(Exception $e)
{
	$con->rollback();
	echo("Error nuking tables\n");
}


$con =null;

?>
