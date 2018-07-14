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
		$sig = base64_decode($headers["X-ident-signature"]);
		//headers okay

		$sigfile = tempnam(hiConfGet("tempPath"), "hisig");
		file_put_contents($sigfile, $sig);

		$enc=  base64_encode(json_encode(httpi_serialise_request()));
		
		$command = escapeshellcmd(hiConfGet("pythonExecutablePath") ." ". hiConfGet("cgiLocation")."/verifysig.py \"".$enc."\" \"$sigfile\" \"".hiConfGet("gpgHome")."\"");
		$keyinfo = json_decode(substr(shell_exec($command."  2>&1; echo $?"),0,-3));
		unlink($sigfile);

		if($keyinfo==null) 
			die403();
		else {
			echo(json_encode($keyinfo));
		}
	}
	else die403();
}
else die403();

?>
