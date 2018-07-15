<?php
require_once "../httpi.inc";
//TODO Add options for displaying

$con = httpi_connect();

$r = $con->query("SELECT * FROM accesslog");

function getip($con, $field)
{
	$st = $con->prepare("SELECT INET_NTOA(:ip)");
	$st->execute(array("ip"=>$field));
	
	return $st->fetch()[0];
}

function keyid($con, $id)
{
	$st = $con->prepare("SELECT authed_by FROM authlog WHERE id = :id");
	$st->execute(array("id"=>$id));
	if($v = $st->fetch())
	{
		return $v["authed_by"];
	}else return "(unbound)";
}

function tokenname($con, $id)
{
	$st = $con->prepare("SELECT token FROM tokens WHERE id = :id");
	$st->execute(array("id"=>$id));
	if($v = $st->fetch())
	{
		return $v["token"];
	}else return "(unbound)";
}

function levelcol($lv)
{
	if($lv == HTTPI_AL_LOW) return "\x1b[36m";
	elseif($lv == HTTPI_AL_WARN) return "\x1b[33m";
	elseif($lv == HTTPI_AL_HIGH) return "\x1b[31m";
}


$fancy = hiConfGet("mFancyTtyOutput", null, false);

function fmsg($type)
{
	global $fancy;
	if($fancy)
	return levelcol(hial_level($type)).hial_message($type)."\x1b[0m";
	else return  hial_message($type);
}

function fkn($key)
{
	global $fancy;
	if($fancy)
		return "\x1b[32m$key\x1b[0m";
	else
		return $key;
}

while($line = $r->fetch())
{
	$ip = getip($con, $line["who"]);
	$type = $line["type"];
	echo("[".$line["id"]." ".$line["timestamp"]."] (".$ip.") at ".($line["is_ident"]==1?"(ident) ":"").$line["host"]."\"".$line["url"]."\" (".$line["query_string"]."):\t[".hial_name($type)."] ".fmsg($type)." (level ".hial_level($type).")");
	if($line["auth_id"]!=null)
		echo(" with key ".fkn(keyid($con, $line["auth_id"])));
	if($line["token_id"]!=null)
		echo(" with token ".tokenname($con, $line["token_id"]));
	echo("\n");
}

?>
