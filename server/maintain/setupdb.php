<?php
require_once "../httpi.inc";

$con = httpi_connect();

if($con==null) die("Could not connect to database\n");

if(!httpi_setup_database($con))
	echo("Error creating tables: (". $con->errno.") ".$con->error."\n");

$con->close();

?>
