<?php

$hiConfig = array();

function hiConfSet($name, $value)
{
	global $hiConfig;
	$hiConfig[$name] = $value;
}

function hiConfGet($name,$or=null)
{
	global $hiConfig;
	if(isset($hiConfig[$name])) return $hiConfig[$name];
	else return $or;
}

?>
