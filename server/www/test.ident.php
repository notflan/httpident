<?php
require_once "../httpi.inc";

$con = httpi_connect();

$qs = "?".$_SERVER["QUERY_STRING"];
if($qs=="?") $qs = "";

$tokdata = httpi_create_token($con, "/test".$qs, $_SERVER["REMOTE_ADDR"], null);

echo(json_encode($tokdata));
?>
