<?php

$hiConfig = array();

function hiConfSet($name, $value)
{
	global $hiConfig;
	$hiConfig[$name] = $value;
}

function hiConfGet($name,$instance=null,$or=null)
{
	global $hiConfig;
	if($instance!=null && isset($instance[$name])) return $instance[$name];
	
	if(isset($hiConfig[$name])) return $hiConfig[$name];
	else return $or;
	
}

function hiParseHeaders()
{
	$headers = array();
	foreach ($_SERVER as $key => $value) {
		if (strpos($key, 'HTTP_') === 0) {
			$headers[str_replace(' ', '', ucwords(str_replace('_', '-', strtolower(substr($key, 5)))))] = $value;
   		}
	}
	return $headers;
}

function hiRequest()
{
	$headers = array();
	foreach ($_SERVER as $key => $value) {
		if (strpos($key, 'REQUEST_') === 0) {
			$headers[$key] = $value;
   		}
	}
	return $headers;
}

function hiAccessLog($ins=null)
{
	
}

?>
