<?php
require_once "../httpi.inc";

function die403()
{
	header("HTTP/1.1 403 Forbidden");
	die("403 Forbidden");
}

$headers = hiParseHeaders();

if(isset($headers["X-ident-token"]))
{
	$token = $headers["X-ident-token"];
	if(isset($headers["X-ident-signature"]))
	{
		$sig = $headers["X-ident-signature"];
		//headers okay

		$enc=  base64_encode(json_encode(httpi_serialise_request()));
		
		$command = escapeshellcmd(hiConfGet("pythonExecutablePath") ." ". hiConfGet("cgiLocation")."/verifysig.py \"".$enc."\"");
		$output = shell_exec($command."  2>&1; echo $?");
		echo $output;
		//echo(json_encode(httpi_serialise_request() ));
	}
	else die403();
}
else die403();

?>
