<?php
require_once "../httpi.inc";

$con = httpi_connect();

$tokdata = httpi_create_token($con, "/test", "127.0.0.1", null);

echo(json_encode($tokdata));
?>
