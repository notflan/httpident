<?php
require_once "httpi-tools.inc";

//Access logging levels
define("HTTPI_AL_LOW", 0);
define("HTTPI_AL_WARN", 1);
define("HTTPI_AL_HIGH", 2);

//Access logging types
define("HTTPI_A_IDENT_REQ", 1);
define("HTTPI_A_AUTH", 2);
define("HTTPI_A_BAD_AUTH_ATTEMPT", 3);
define("HTTPI_A_INVALID_HEADER_AUTH_ATTEMPT", 4);
define("HTTPI_A_IDENT_REQ_DENIED", 5);
define("HTTPI_A_INVALID_TOKEN_AUTH_ATTEMPT", 6);
define("HTTPI_A_EXPIRED_TOKEN_AUTH_ATTEMPT", 7);
define("HTTPI_A_NOSIG_AUTH_ATTEMPT", 8);

$gAccessLogInfo = array();

_hial_init(HTTPI_A_IDENT_REQ, "HTTPI_A_IDENT_REQ", HTTPI_AL_LOW, "Ident token request");
_hial_init(HTTPI_A_AUTH, "HTTPI_A_AUTH", HTTPI_AL_LOW, "Client authenticated");
_hial_init(HTTPI_A_BAD_AUTH_ATTEMPT, "HTTPI_A_BAD_AUTH_ATTEMPT", HTTPI_AL_WARN, "Bad auth attempt");
_hial_init(HTTPI_A_INVALID_HEADER_AUTH_ATTEMPT, "HTTPI_A_INVALID_HEADER_AUTH_ATTEMPT", HTTPI_AL_LOW, "Client attempted to acces page without headers");
_hial_init(HTTPI_A_IDENT_REQ_DENIED, "HTTPI_A_IDENT_REQ_DENIED", HTTPI_AL_HIGH, "Ident token request denied");
_hial_init(HTTPI_A_INVALID_TOKEN_AUTH_ATTEMPT, "HTTPI_A_INVALID_TOKEN_AUTH_ATTEMPT", HTTPI_AL_HIGH, "Client attempted to access page with invalid token");
_hial_init(HTTPI_A_EXPIRED_TOKEN_AUTH_ATTEMPT, "HTTPI_A_EXPIRED_TOKEN_AUTH_ATTEMPT", HTTPI_AL_WARN, "Client attempted to access page with expired token");
_hial_init(HTTPI_A_NOSIG_AUTH_ATTEMPT, "HTTPI_A_NOSIG_AUTH_ATTEMPT", HTTPI_AL_HIGH, "Client attempted to authenticate with no signature");

function _hial_init($id, $name, $level, $message)
{
	global $gAccessLogInfo;
	$gAccessLogInfo[$id] = array(
		"name" => $name,
		"level"=>$level,
		"message"=>$message
	);
}

function hial_worth($id, $ins=null)
{
	if(!hiConfGet("mEnableAccessLog", $ins, false)) return false;

	global $gAccessLogInfo;
	$lv = hiConfGet("mLoggingLevel", $ins);
	
	return ($gAccessLogInfo[$id]["level"] >= $lv);
}

function hial_name($id)
{
	global $gAccessLogInfo;
	return $gAccessLogInfo[$id]["name"];
}


function hial_level($id)
{
	global $gAccessLogInfo;
	return $gAccessLogInfo[$id]["level"];
}


function hial_message($id)
{
	global $gAccessLogInfo;
	return $gAccessLogInfo[$id]["message"];
}

?>
