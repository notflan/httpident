<?php
require_once "httpi-tools.inc";
require_once "conf.inc";
require_once "logging.inc";

//Expiry reasons (expire_reason) for tokens
define("HTTPI_EXR_UNKNOWN", 0);		//Token has not expired or the reason is unknown
define("HTTPI_EXR_AUTH_SUCCESS", 1);	//Token has been successfully used for auth
define("HTTPI_EXR_AUTH_FAILURE", 2);	//Token has been unsuccessfully used for auth
define("HTTPI_EXR_TIMEOUT", 3);		//Token has reached timeout
define("HTTPI_EXR_INVALID_USE", 4);	//Token was attempted to be used for invalid auth (ip/url mismatch)
define("HTTPI_EXR_ALREADY_EXPIRED", 5);


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
	) ENGINE=".hiConfGet("dbEngine"));
	if(!$ok) return $ok; else
	$ok = $con->query("CREATE TABLE IF NOT EXISTS authlog (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		token_id INT NOT NULL,
		authed_by VARCHAR(16) DEFAULT NULL,
		timestamp TIMESTAMP NOT NULL,
		index by_token (token_id),
		CONSTRAINT c_token
		FOREIGN KEY fk_token(token_id)
		REFERENCES tokens(id)
		ON UPDATE CASCADE
		ON DELETE RESTRICT
	) ENGINE=".hiConfGet("dbEngine"));
	if(!$ok) return $ok; else
	$ok = $con->query("CREATE TABLE IF NOT EXISTS accesslog (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		who INT UNSIGNED NOT NULL,
		timestamp TIMESTAMP NOT NULL,
		url VARCHAR(128) NOT NULL,
		type INT NOT NULL DEFAULT 0,
		auth_id INT NULL,
		token_id INT NULL,
		headers VARCHAR(1024) DEFAULT NULL,
		query_string VARCHAR(256) DEFAULT NULL,
		host VARCHAR(128) DEFAULT NULL,
		is_ident BOOL NOT NULL DEFAULT FALSE,
		index by_type (type),

		CONSTRAINT c_auth
		FOREIGN KEY fk_auth(auth_id)
		REFERENCES authlog(id)
		ON UPDATE CASCADE
		ON DELETE RESTRICT,

		CONSTRAINT ac_token
		FOREIGN KEY afk_token(token_id)
		REFERENCES tokens(id)
		ON UPDATE CASCADE
		ON DELETE RESTRICT

	) ENGINE=".hiConfGet("dbEngine"));
	return $ok;
}

function httpi_create_token($con, $url, $ip, $expires_at=null)
{
	$tok = httpi_generate_token();

	if($expires_at!=null)
		$expires_at = date("Y-m-d H:i:s", time()+$expires_at);

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

function httpi_valid_token($con, $tok, $url, $ip)
{
	$st = $con->prepare("SELECT INET_NTOA(who) as who, url, expires_at, expired FROM tokens WHERE token = :token");
	$st->execute(array("token"=>$tok));
	if($row = $st->fetch())
	{
		//die($ip." ".$row["who"]." - ".$url." ".$row["url"]);
		if($row["expired"]) return HTTPI_EXR_ALREADY_EXPIRED;
		if($row["who"]!=$ip) return HTTPI_EXR_INVALID_USE;
		if($row["url"]!=$url) return HTTPI_EXR_INVALID_USE;
		if($row["expires_at"]!=null && (time()>strtotime($row["expires_at"])))
			return HTTPI_EXR_TIMEOUT;
		
		return true;
	}
	else return HTTPI_EXR_UNKNOWN;
}

function _httpi_get_token_id($con, $tok)
{
	$st = $con->prepare("SELECT id FROM tokens WHERE token = :token");
	$st->execute(array("token"=> $tok));
	if($vt = $st->fetch())
	{
		return $vt["id"]; 
	}else return null;
}

function _httpi_get_auth_id_by_token($con, $tok)
{
	$tid = _httpi_get_token_id($con, $tok);
	if($tid==null) return null;
	$st = $con->prepare("SELECT id FROM authlog WHERE token_id = :tokenid");
	$st->execute(array("tokenid"=>$tid));
	if($vt = $st->fetch())
	{
		return $vt["id"];
	}
	else return null;
}

function httpi_expire_token($con, $tok, $why, $key=null)
{	
	$id = _httpi_get_token_id($con, $tok);
	if($id==null) return false;
	try {
		$con->beginTransaction();
		$st = $con->prepare("UPDATE tokens SET `expired` = TRUE, `expire_reason` = :reason WHERE `id` = :id");
		$st->execute(array( "reason" => $why, "id" => $id));
		if($key!=null) {
			$ist = $con->prepare("INSERT INTO authlog(token_id, authed_by) VALUES (:id, :ab)");
			$ist->execute(array("id"=>$id, "ab" => $key));
			
		}
		$con->commit();
		return true;
	}catch(Exception $e)
	{
		$con->rollback();
		return false;
	}
	return false;
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

function die403()
{
	header("HTTP/1.1 403 Forbidden");
	die(hiConfGet("m403Message", null, "403 Forbidden"));
}

function httpi_auth_page()
{
	$headers = hiParseHeaders();
	$con = httpi_connect();

	if(isset($headers["X-ident-token"]))
	{
		$token = $headers["X-ident-token"];

		if(isset($headers["X-ident-signature"]))
		{
			$sig = base64_decode($headers["X-ident-signature"]);
			//headers okay
			$vd = httpi_valid_token($con, $token, $_SERVER["REQUEST_URI"], $_SERVER['REMOTE_ADDR']);

			if($vd===true) {

				$sigfile = tempnam(hiConfGet("tempPath"), "hisig");
				file_put_contents($sigfile, $sig);

				$enc=  base64_encode(json_encode(httpi_serialise_request()));
				
				$command = escapeshellcmd(hiConfGet("pythonExecutablePath") ." ". hiConfGet("cgiLocation")."/verifysig.py \"".$enc."\" \"$sigfile\" \"".hiConfGet("gpgHome")."\"");
				$keyinfo = json_decode(substr(shell_exec($command."  2>&1; echo $?"),0,-3), true);
				unlink($sigfile);

				if($keyinfo==null) {
					httpi_expire_token($con, $token, HTTPI_EXR_AUTH_FAILURE);
					_httpi_attempt_log($con, HTTPI_A_BAD_AUTH_ATTEMPT, false, null, _httpi_get_token_id($con, $token));
					die403();
				}
				else {
					httpi_auth_token($con, $token, $keyinfo["key_id"]);
					_httpi_attempt_log($con, HTTPI_A_AUTH, false, _httpi_get_auth_id_by_token($con, $token), _httpi_get_token_id($con, $token));
					return $keyinfo;
				}
			}
			else {
				httpi_expire_token($con, $token, $vd);
				if($vd == HTTPI_EXR_UNKNOWN)
					_httpi_attempt_log($con, HTTPI_A_INVALID_TOKEN_AUTH_ATTEMPT, false);
				elseif($vd== HTTPI_EXR_ALREADY_EXPIRED || $vd == HTTPI_EXR_TIMEOUT)
					_httpi_attempt_log($con, HTTPI_A_EXPIRED_TOKEN_AUTH_ATTEMPT, false, null, _httpi_get_token_id($con, $token));
				else
					_httpi_attempt_log($con, HTTPI_A_INVALID_TOKEN_AUTH_ATTEMPT, false, null, _httpi_get_token_id($con, $token));
				die403();
			}
		}
		else {
			httpi_expire_token($con, $token, HTTPI_EXR_INVALID_USE);
			_httpi_attempt_log($con, HTTPI_A_NOSIG_AUTH_ATTEMPT, false,null,_httpi_get_token_id($con, $token));
			die403();
		}
	}
	else {
		_httpi_attempt_log($con, HTTPI_A_INVALID_HEADER_AUTH_ATTEMPT, false);
		die403();
	}
}

function httpi_ident_page($redi, $ins=null)
{
	$con = httpi_connect();

	$qs = "?".$_SERVER["QUERY_STRING"];
	if($qs=="?") $qs = "";

	$tokdata = httpi_create_token($con, $redi.$qs, $_SERVER["REMOTE_ADDR"], hiConfGet("tokExpiresIn", $ins));

	if($tokdata!=null)
		_httpi_attempt_log($con, HTTPI_A_IDENT_REQ, true,null, _httpi_get_token_id($con, $tokdata["token"]), $ins);
	else
		_httpi_attempt_log($con, HTTPI_A_IDENT_REQ_DENIED, true, null, null, $ins);

	echo(json_encode($tokdata));
}

function _httpi_attempt_log($con, $type, $is_ident, $auth_id=null, $token_id=null, $ins=null)
{
	if(hial_worth($type, $ins))
	{
		$st = $con->prepare("INSERT INTO accesslog (who, url, type, auth_id, token_id, headers, query_string, host, is_ident) VALUES (INET_ATON(:ip), :url, :type, :auth, :token, :headers, :query, :host, :ident)");
		$st->execute(array(
			"ip"=> $_SERVER["REMOTE_ADDR"],
			"url"=>$_SERVER["REQUEST_URI"],
			"type"=>$type,
			"auth"=>$auth_id,
			"token"=>$token_id,
			"headers"=>json_encode(hiParseHeaders()),
			"query"=>$_SERVER["QUERY_STRING"],
			"host"=>$_SERVER["SERVER_NAME"],
			"ident"=>$is_ident?1:0
		));	
		return true;
	}
	return false;
}

?>
