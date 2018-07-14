<?php
require_once "httpi-tools.inc";
require_once "conf.inc";

//Expiry reasons (expire_reason) for tokens
define("HTTPI_EXR_UNKNOWN", 0);		//Token has not expired or the reason is unknown
define("HTTPI_EXR_AUTH_SUCCESS", 1);	//Token has been successfully used for auth
define("HTTPI_EXR_AUTH_FAILURE", 2);	//Token has been unsuccessfully used for auth
define("HTTPI_EXR_TIMEOUT", 3);		//Token has reached timeout
define("HTTPI_EXR_INVALID_USE", 4);	//Token was attempted to be used for invalid auth (ip/url mismatch)

function httpi_generate_token()
{
	return hash("sha256",microtime()."".random_bytes(100));
}

function httpi_connect_to($host, $user, $password, $database)
{
	try {
		$con = new PDO("mysql:host=$host;dbname=$database", $user, $password);
		return $con;
	}
	catch(PDOException $e)
	{
		return null;
	} 
}

function httpi_connect()
{
	return httpi_connect_to(hiConfGet("dbHost"), hiConfGet("dbUser"), hiConfGet("dbPasswd"), hiConfGet("dbName"));
}

function httpi_setup_database($con)
{
	$ok = $con->query("CREATE TABLE IF NOT EXISTS tokens (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		token VARCHAR(64) NOT NULL,
		url VARCHAR(128) NOT NULL,
		who INT UNSIGNED NOT NULL,
		timestamp TIMESTAMP NOT NULL,
		expires_at DATETIME DEFAULT NULL,
		expired BOOL DEFAULT FALSE NOT NULL,
		expire_reason INT DEFAULT 0 NOT NULL,
		INDEX by_token (token),
		CONSTRAINT token_unique UNIQUE (token)
	) ENGINE=InnoDB");
	if(!$ok) return false; else
	$ok = $con->query("CREATE TABLE IF NOT EXISTS authlog (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		token_id INT NOT NULL,
		authed_by VARCHAR(8) DEFAULT NULL,
		index by_token (token_id),
		CONSTRAINT c_token
		FOREIGN KEY fk_token(token_id)
		REFERENCES tokens(id)
		ON UPDATE CASCADE
		ON DELETE RESTRICT
	) ENGINE=InnoDB");
	return $ok;
}

function httpi_create_token($con, $url, $ip, $expires_at=null)
{
	$tok = httpi_generate_token();

	$st = $con->prepare("INSERT INTO tokens (token, url, who, expires_at) VALUES (
		:token,
		:url,
		INET_ATON(:who),
		:expires_at)");
	$st->execute(array(
		"token" => $tok,
		"url" => $url,
		"who" => $ip,
		"expires_at" => $expires_at
	));


	return array(
		'token'=>$tok,
		'for'=>$url,
		'who'=>$ip,
		'until'=> $expires_at,
	);
}

function _httpi_get_token_id($con, $tok)
{
	$st = $con->prepare("SELECT id FROM tokens WHERE token = :token");
	$ret = $st->execute(array("token"=> $tok));
	$assoc= $ret->fetchAll();
	return $assoc[0]["id"];
}

function httpi_expire_token($con, $tok, $why, $key=null)
{	
	$id = _httpi_get_token_id($con, $tok);
	try {
		$con->beginTransaction();
		$st = $con->prepare("UPDATE tokens SET expired = TRUE, expired_reason = :reason WHERE token = :token");
		$st->execute(array( "reason" => $why, "token" => $tok));
		if($key!=null) {
			$ist = $con->prepare("INSERT INTO authlog (token_id, authed_by) VALUES (:id, :ab)");
			$ist->execute(array("id"=>$id, "ab" => $key));
		}
		$con->commit();	
	}catch(Exception $e)
	{
		$con->rollback();
		return $e;
	}
	return null;
}

function httpi_auth_token($con, $tok, $key)
{
	httpi_expire_token($con,$tok, HTTPI_EXR_AUTH_SUCCESS, $key);
}

function httpi_serialise_request()
{
	$head = hiParseHeaders();
	unset($head["X-ident-signature"]);
	$ar =  array(
		"method" => $_SERVER['REQUEST_METHOD'],
		"type" => $_SERVER["REQUEST_SCHEME"],
		"host" => $_SERVER["SERVER_NAME"],
		"uri" => $_SERVER["REQUEST_URI"],
		"headers" => $head,
		"data" => file_get_contents('php://input')
	);	
	if($ar["data"]=="") $ar["data"]=null;
	return $ar;
}

?>
