<?php
require_once "../httpi.inc";

$con = httpi_connect();

if($con==null) die("Could not connect to database\n");

$con->setAttribute( PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING );

if(!httpi_setup_database($con)) {
	echo("Error creating tables: ");
	print_r($con->errorInfo());
}
$con =null;

?>
