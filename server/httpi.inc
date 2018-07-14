<?php
require_once "httpi-tools.inc";
require_once "conf.inc";

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
	) ENGINE=InnoDB");
	if(!$ok) return false; else
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
	die("403 Forbidden");
}

function httpi_auth_page()
{
	$headers = hiParseHeaders();

	if(isset($headers["X-ident-token"]))
	{
		$token = $headers["X-ident-token"];
		$con = httpi_connect();

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
					die403();
				}
				else {
					httpi_auth_token($con, $token, $keyinfo["key_id"]);
					return $keyinfo;
				}
			}
			else {
				httpi_expire_token($con, $token, $vd);
				die403();
			}
		}
		else {
			httpi_expire_token($con, $token, HTTPI_EXR_INVALID_USE);
			die403();
		}
	}
	else die403();
}

?>
