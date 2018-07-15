<?php
require_once "../httpi.inc";

$con = httpi_connect();

try {
	$con->query("TRUNCATE TABLE accesslog");
	echo("Done\n");
}catch(Exeption $e){
	echo("Could not truncate table\n");
}

?>
